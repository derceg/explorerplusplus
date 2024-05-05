// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "iDirectoryMonitor.h"
#include "Macros.h"
#include <list>

DWORD WINAPI Thread_DirModifiedInternal(LPVOID Container);

class DirectoryMonitor : public IDirectoryMonitor
{
public:
	DirectoryMonitor();
	~DirectoryMonitor();

	HRESULT __stdcall QueryInterface(REFIID iid, void **ppvObject);
	ULONG __stdcall AddRef();
	ULONG __stdcall Release();

	std::optional<int> WatchDirectory(const TCHAR *Directory, UINT WatchFlags,
		OnDirectoryAltered onDirectoryAltered, BOOL bWatchSubTree, void *pData) override;
	std::optional<int> WatchDirectory(HANDLE hDirectory, const TCHAR *Directory, UINT WatchFlags,
		OnDirectoryAltered onDirectoryAltered, BOOL bWatchSubTree, void *pData) override;
	BOOL StopDirectoryMonitor(int iStopId) override;

private:
#define PRIMARY_BUFFER_SIZE 1000 * sizeof(FILE_NOTIFY_INFORMATION)

	/* These function are only ever queued as APC's. */
	static void CALLBACK WatchAndCreateDirectoryInternal(ULONG_PTR dwParam);
	static void CALLBACK CompletionRoutine(DWORD dwErrorCode, DWORD NumberOfBytesTransferred,
		LPOVERLAPPED lpOverlapped);
	static void CALLBACK StopDirectoryWatch(ULONG_PTR dwParam);

	/* This function is called as an APC, and stops
	the worker thread on exit. */
	static void CALLBACK ExitWorkerThread(ULONG_PTR dwParam);

	/* These are only called in the context of the
	worker thread (i.e. they are called from APC's,
	but are not queued as APC's themselves). */
	static void CALLBACK WatchDirectoryInternal(ULONG_PTR dwParam);
	static void DeleteRequest(ULONG_PTR dwParam);

	class DirInfo
	{
	public:
		OnDirectoryAltered m_OnDirectoryAltered;

		DirectoryMonitor *m_pDirectoryMonitor;
		FILE_NOTIFY_INFORMATION *m_FileNotifyBuffer;
		HANDLE m_hThread;
		HANDLE m_hDirectory;
		OVERLAPPED m_Async;
		LPVOID m_pData;
		TCHAR m_DirPath[MAX_PATH];
		UINT m_WatchFlags;
		BOOL m_bWatchSubTree;
		BOOL m_bMarkedForDeletion;
		BOOL m_bDirMonitored;
		int m_UniqueId;
	};

	int m_iRefCount;
	DWORD m_ThreadId;
	HANDLE m_hThread;
	std::list<DirInfo> m_DirWatchInfoList;
	CRITICAL_SECTION m_cs;
	HANDLE m_hStopThreadEvent;
	int m_UniqueId;
};

HRESULT CreateDirectoryMonitor(IDirectoryMonitor **pDirectoryMonitor)
{
	*pDirectoryMonitor = new DirectoryMonitor();

	return S_OK;
}

DirectoryMonitor::DirectoryMonitor()
{
	m_iRefCount = 1;
	m_UniqueId = 0;
	m_hThread = CreateThread(nullptr, 0, Thread_DirModifiedInternal, nullptr, 0, &m_ThreadId);

	InitializeCriticalSection(&m_cs);

	m_hStopThreadEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
}

DirectoryMonitor::~DirectoryMonitor()
{
	/* Queue the APC that will stop the worker
	thread. */
	QueueUserAPC(ExitWorkerThread, m_hThread, (ULONG_PTR) this);
	CloseHandle(m_hThread);

	/* Wait for the worker thread to stop. Once it
	has been stopped, delete any dependent data
	(such as any critical sections). */
	WaitForSingleObject(m_hStopThreadEvent, INFINITE);

	DeleteCriticalSection(&m_cs);
	m_DirWatchInfoList.clear();
	CloseHandle(m_hStopThreadEvent);
}

/* IUnknown interface members. */
HRESULT __stdcall DirectoryMonitor::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = nullptr;

	if (iid == IID_IUnknown)
	{
		*ppvObject = this;
	}

	if (*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall DirectoryMonitor::AddRef()
{
	return ++m_iRefCount;
}

ULONG __stdcall DirectoryMonitor::Release()
{
	m_iRefCount--;

	if (m_iRefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_iRefCount;
}

DWORD WINAPI Thread_DirModifiedInternal(LPVOID lpParameter)
{
	UNREFERENCED_PARAMETER(lpParameter);

	/* This is the main thread. Sleeps until woken up by a
	change notification, and then goes back to sleep. */

	SetErrorMode(SEM_FAILCRITICALERRORS);

/* Warning C4127 (conditional expression is
constant) temporarily disabled for this function. */
#pragma warning(push)
#pragma warning(disable : 4127)
	while (TRUE)
	{
		SleepEx(INFINITE, TRUE);
	}
#pragma warning(pop)

	return 0;
}

void CALLBACK DirectoryMonitor::ExitWorkerThread(ULONG_PTR dwParam)
{
	DirectoryMonitor *pdm = nullptr;

	pdm = (DirectoryMonitor *) dwParam;

	/* Set the event indicating that it is now
	safe to destroy data that is shared between
	threads. */
	SetEvent(pdm->m_hStopThreadEvent);

	ExitThread(0);
}

std::optional<int> DirectoryMonitor::WatchDirectory(const TCHAR *Directory, UINT WatchFlags,
	OnDirectoryAltered onDirectoryAltered, BOOL bWatchSubTree, void *pData)
{
	DirInfo pDirInfo;

	if (Directory == nullptr)
	{
		return std::nullopt;
	}

	pDirInfo.m_pDirectoryMonitor = this;
	pDirInfo.m_hThread = m_hThread;
	pDirInfo.m_WatchFlags = WatchFlags;
	pDirInfo.m_UniqueId = m_UniqueId;
	pDirInfo.m_OnDirectoryAltered = onDirectoryAltered;
	pDirInfo.m_pData = pData;
	pDirInfo.m_bWatchSubTree = bWatchSubTree;
	pDirInfo.m_bMarkedForDeletion = FALSE;

	/* This suppresses crtical error message boxes, such as the one
	that mey arise from CreateFile() when opening attempting to
	open a floppy drive that doesn't have a floppy disk (also
	CD/DVD drives etc). */
	SetErrorMode(SEM_FAILCRITICALERRORS);

	StringCchCopy(pDirInfo.m_DirPath, std::size(pDirInfo.m_DirPath), Directory);

	pDirInfo.m_hDirectory = CreateFile(pDirInfo.m_DirPath, FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);

	if (pDirInfo.m_hDirectory == INVALID_HANDLE_VALUE)
	{
		free(pData);
		return std::nullopt;
	}

	EnterCriticalSection(&m_cs);

	/* TODO: This operation alters the list, including the
	previously inserted item (e.g by updating list pointers).
	Therefore, need to lock access between this function and
	other reads of the inserted item. */
	m_DirWatchInfoList.push_back(pDirInfo);

	m_DirWatchInfoList.back().m_Async.hEvent = &m_DirWatchInfoList.back();
	QueueUserAPC(WatchDirectoryInternal, m_hThread, (ULONG_PTR) &m_DirWatchInfoList.back());

	LeaveCriticalSection(&m_cs);

	return m_UniqueId++;
}

std::optional<int> DirectoryMonitor::WatchDirectory(HANDLE hDirectory, const TCHAR *Directory,
	UINT WatchFlags, OnDirectoryAltered onDirectoryAltered, BOOL bWatchSubTree, void *pData)
{
	DirInfo pDirInfo;

	if (Directory == nullptr)
	{
		return std::nullopt;
	}

	pDirInfo.m_pDirectoryMonitor = this;
	pDirInfo.m_hThread = m_hThread;
	pDirInfo.m_WatchFlags = WatchFlags;
	pDirInfo.m_UniqueId = m_UniqueId;
	pDirInfo.m_OnDirectoryAltered = onDirectoryAltered;
	pDirInfo.m_pData = pData;
	pDirInfo.m_bWatchSubTree = bWatchSubTree;
	pDirInfo.m_bMarkedForDeletion = FALSE;

	/* This suppresses crtical error message boxes, such as the one
	that mey arise from CreateFile() when opening attempting to
	open a floppy drive that doesn't have a floppy disk (also
	CD/DVD drives etc). */
	SetErrorMode(SEM_FAILCRITICALERRORS);

	StringCchCopy(pDirInfo.m_DirPath, std::size(pDirInfo.m_DirPath), Directory);

	pDirInfo.m_hDirectory = hDirectory;

	EnterCriticalSection(&m_cs);

	m_DirWatchInfoList.push_back(pDirInfo);
	m_DirWatchInfoList.back().m_Async.hEvent = &m_DirWatchInfoList.back();
	// QueueUserAPC(WatchAndCreateDirectoryInternal,m_hThread,(ULONG_PTR)&m_DirWatchInfoList.back());
	QueueUserAPC(WatchDirectoryInternal, m_hThread, (ULONG_PTR) &m_DirWatchInfoList.back());

	LeaveCriticalSection(&m_cs);

	return m_UniqueId++;
}

/* These functions (CreateFile and ReadDirectoryChangesW) DO NOT appear
to put the thread in an alertable wait state. */
void CALLBACK DirectoryMonitor::WatchAndCreateDirectoryInternal(ULONG_PTR dwParam)
{
	DirInfo *pDirInfo = nullptr;

	pDirInfo = reinterpret_cast<DirInfo *>(dwParam);

	pDirInfo->m_hDirectory = CreateFile(pDirInfo->m_DirPath, FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);

	WatchDirectoryInternal(dwParam);
}

void DirectoryMonitor::WatchDirectoryInternal(ULONG_PTR dwParam)
{
	DirInfo *pDirInfo = nullptr;

	pDirInfo = reinterpret_cast<DirInfo *>(dwParam);

	if (pDirInfo->m_hDirectory == INVALID_HANDLE_VALUE)
	{
		free(pDirInfo->m_pData);

		return;
	}

	pDirInfo->m_FileNotifyBuffer = (FILE_NOTIFY_INFORMATION *) malloc(PRIMARY_BUFFER_SIZE);

	pDirInfo->m_bDirMonitored = ReadDirectoryChangesW(pDirInfo->m_hDirectory,
		pDirInfo->m_FileNotifyBuffer, PRIMARY_BUFFER_SIZE, pDirInfo->m_bWatchSubTree,
		pDirInfo->m_WatchFlags, nullptr, &pDirInfo->m_Async, CompletionRoutine);

	if (!pDirInfo->m_bDirMonitored)
	{
		free(pDirInfo->m_FileNotifyBuffer);
		CancelIo(pDirInfo->m_hDirectory);
		CloseHandle(pDirInfo->m_hDirectory);
	}
}

void CALLBACK DirectoryMonitor::CompletionRoutine(DWORD dwErrorCode, DWORD NumberOfBytesTransferred,
	LPOVERLAPPED lpOverlapped)
{
	DirInfo *pDirInfo = nullptr;
	FILE_NOTIFY_INFORMATION *pfni = nullptr;
	TCHAR szFileName[MAX_PATH];

	if ((dwErrorCode == ERROR_SUCCESS) && (NumberOfBytesTransferred != 0))
	{
		if (lpOverlapped->hEvent == nullptr)
		{
			return;
		}

		pDirInfo = reinterpret_cast<DirInfo *>(lpOverlapped->hEvent);

		pfni = pDirInfo->m_FileNotifyBuffer;
		int i = 0;

		do
		{
			if (i != 0)
			{
				pfni = (FILE_NOTIFY_INFORMATION *) ((LPBYTE) pfni + pfni->NextEntryOffset);
			}

			/* FileNameLength is size in bytes NOT characters. */
			StringCchCopyN(szFileName, SIZEOF_ARRAY(szFileName), pfni->FileName,
				pfni->FileNameLength / sizeof(TCHAR));
			pDirInfo->m_OnDirectoryAltered(szFileName, pfni->Action, pDirInfo->m_pData);

			i++;
		} while (pfni->NextEntryOffset != 0);

		free(pDirInfo->m_FileNotifyBuffer);

		pDirInfo->m_FileNotifyBuffer = nullptr;

		/* Rewatch the directory. */
		WatchDirectoryInternal((ULONG_PTR) pDirInfo);
	}
	else if (dwErrorCode == ERROR_OPERATION_ABORTED)
	{
		pDirInfo = reinterpret_cast<DirInfo *>(lpOverlapped->hEvent);

		/* The specified directory has stop been watched.
		Remove its entry from the queue. */
		DeleteRequest((ULONG_PTR) pDirInfo);
	}
}

void DirectoryMonitor::DeleteRequest(ULONG_PTR dwParam)
{
	DirInfo *pDirInfo = nullptr;
	DirectoryMonitor *pDirectoryMonitor = nullptr;
	std::list<DirInfo>::iterator itr;

	pDirInfo = reinterpret_cast<DirInfo *>(dwParam);

	pDirectoryMonitor = pDirInfo->m_pDirectoryMonitor;

	free(pDirInfo->m_FileNotifyBuffer);
	free(pDirInfo->m_pData);

	EnterCriticalSection(&pDirectoryMonitor->m_cs);

	for (itr = pDirectoryMonitor->m_DirWatchInfoList.begin();
		 itr != pDirectoryMonitor->m_DirWatchInfoList.end(); itr++)
	{
		if (itr->m_UniqueId == pDirInfo->m_UniqueId)
		{
			pDirectoryMonitor->m_DirWatchInfoList.erase(itr);
			break;
		}
	}

	LeaveCriticalSection(&pDirectoryMonitor->m_cs);
}

BOOL DirectoryMonitor::StopDirectoryMonitor(int iStopId)
{
	std::list<DirInfo>::iterator itr;

	EnterCriticalSection(&m_cs);

	for (itr = m_DirWatchInfoList.begin(); itr != m_DirWatchInfoList.end(); itr++)
	{
		if (itr->m_UniqueId == iStopId)
		{
			/* Only stop monitoring the directory if it was
			actually monitored in the first place! */
			if (itr->m_bDirMonitored)
			{
				QueueUserAPC(StopDirectoryWatch, m_hThread, (ULONG_PTR) itr->m_hDirectory);
			}

			break;
		}
	}

	LeaveCriticalSection(&m_cs);

	return TRUE;
}

void CALLBACK DirectoryMonitor::StopDirectoryWatch(ULONG_PTR dwParam)
{
	HANDLE hDirectory;

	hDirectory = reinterpret_cast<HANDLE>(dwParam);

	CancelIo(hDirectory);
	CloseHandle(hDirectory);

	hDirectory = nullptr;
}
