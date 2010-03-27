/******************************************************************
 *
 * Project: Helper
 * File: iDirectoryMonitor.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Class to monitor directories for changes (e.g. file renaming,
 * deleting, creation, etc.). Monitoring is performed
 * asynchronously.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "iDirectoryMonitor.h"


using namespace std;

DWORD WINAPI Thread_DirModifiedInternal(LPVOID Container);

class CDirectoryMonitor : public IDirectoryMonitor
{
public:

	CDirectoryMonitor(void);
	~CDirectoryMonitor();

	HRESULT		__stdcall	QueryInterface(REFIID iid, void **ppvObject);
	ULONG		__stdcall	AddRef(void);
	ULONG		__stdcall	Release(void);

	int		WatchDirectory(TCHAR *Directory,UINT WatchFlags,
		void (*DirectoryAltered)(TCHAR *szFileName,DWORD dwAction,void *pData),
		BOOL	bWatchSubTree,void *pData);
	int WatchDirectory(HANDLE hDirectory,TCHAR *Directory,UINT WatchFlags,
		void (*DirectoryAltered)(TCHAR *szFileName,DWORD dwAction,void *pData),
		BOOL bWatchSubTree,void *pData);
	BOOL	StopDirectoryMonitor(int iStopId);

private:

	#define PRIMARY_BUFFER_SIZE 1000 * sizeof(FILE_NOTIFY_INFORMATION)

	/* These function are only ever queued as APC's. */
	static void CALLBACK	WatchAndCreateDirectoryInternal(ULONG_PTR dwParam);
	static void CALLBACK	CompletionRoutine(DWORD dwErrorCode,DWORD NumberOfBytesTransferred,LPOVERLAPPED lpOverlapped);
	static void CALLBACK	StopDirectoryWatch(ULONG_PTR dwParam);

	/* This function is called as an APC, and stops
	the worker thread on exit. */
	static void CALLBACK	ExitWorkerThread(ULONG_PTR dwParam);

	/* These are only called in the context of the
	worker thread (i.e. they are called from APC's,
	but are not queued as APC's themselves). */
	static void CALLBACK	WatchDirectoryInternal(ULONG_PTR dwParam);
	static void				DeleteRequest(ULONG_PTR dwParam);
	static void				CopyDirectoryChangeFileName(FILE_NOTIFY_INFORMATION *pfni,TCHAR *szFileName,size_t iBufLen);

	class CDirInfo
	{
	public:
		void	(*m_DirectoryAltered)(TCHAR *szFileName,DWORD dwAction,void *pData);

		CDirectoryMonitor		*m_pDirectoryMonitor;
		FILE_NOTIFY_INFORMATION	*m_FileNotifyBuffer;
		HANDLE					m_hThread;
		HANDLE					m_hDirectory;
		OVERLAPPED				m_Async;
		LPVOID					m_pData;
		TCHAR					m_DirPath[MAX_PATH];
		UINT					m_WatchFlags;
		BOOL					m_bWatchSubTree;
		BOOL					m_bMarkedForDeletion;
		BOOL					m_bDirMonitored;
		int						m_UniqueId;
	};

	int					m_iRefCount;
	DWORD				m_ThreadId;
	HANDLE				m_hThread;
	list<CDirInfo>		m_DirWatchInfoList;
	CRITICAL_SECTION	m_cs;
	HANDLE				m_hStopThreadEvent;
	int					m_UniqueId;
};

HRESULT CreateDirectoryMonitor(IDirectoryMonitor **pDirectoryMonitor)
{
	*pDirectoryMonitor = new CDirectoryMonitor();

	return S_OK;
}

CDirectoryMonitor::CDirectoryMonitor(void)
{
	m_iRefCount	= 1;
	m_UniqueId	= 0;
	m_hThread	= CreateThread(NULL,0,Thread_DirModifiedInternal,NULL,0,&m_ThreadId);

	InitializeCriticalSection(&m_cs);

	m_hStopThreadEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
}

CDirectoryMonitor::~CDirectoryMonitor()
{
	/* Queue the APC that will stop the worker
	thread. */
	QueueUserAPC(ExitWorkerThread,m_hThread,(ULONG_PTR)this);

	/* Wait for the worker thread to stop. Once it
	has been stopped, delete any dependent data
	(such as any critical sections). */
	WaitForSingleObject(m_hStopThreadEvent,INFINITE);

	DeleteCriticalSection(&m_cs);
	m_DirWatchInfoList.clear();
	CloseHandle(m_hStopThreadEvent);
}

/* IUnknown interface members. */
HRESULT __stdcall CDirectoryMonitor::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = NULL;

	if(iid == IID_IUnknown)
	{
		*ppvObject=this;
	}

	if(*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall CDirectoryMonitor::AddRef(void)
{
	return ++m_iRefCount;
}

ULONG __stdcall CDirectoryMonitor::Release(void)
{
	m_iRefCount--;

	if(m_iRefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_iRefCount;
}

DWORD WINAPI Thread_DirModifiedInternal(LPVOID lpParameter)
{
	/* This is the main thread. Sleeps until woken up by a
	change notification, and then goes back to sleep. */

	SetErrorMode(SEM_FAILCRITICALERRORS);

	/* Warning C4127 (conditional expression is
	constant) temporarily disabled for this funtion. */
	#pragma warning( disable : 4127 )
	while(TRUE)
	{
		SleepEx(INFINITE,TRUE);
	}

	return 0;
}

void CALLBACK CDirectoryMonitor::ExitWorkerThread(ULONG_PTR dwParam)
{
	CDirectoryMonitor	*pdm = NULL;

	pdm = (CDirectoryMonitor *)dwParam;

	/* Set the event indicating that it is now
	safe to destroy data that is shared between
	threads. */
	SetEvent(pdm->m_hStopThreadEvent);

	ExitThread(0);
}

int CDirectoryMonitor::WatchDirectory(TCHAR *Directory,UINT WatchFlags,
void (*DirectoryAltered)(TCHAR *szFileName,DWORD dwAction,void *pData),
BOOL bWatchSubTree,void *pData)
{
	CDirInfo	pDirInfo;

	if(Directory == NULL)
		return -1;

	pDirInfo.m_pDirectoryMonitor	= this;
	pDirInfo.m_hThread				= m_hThread;
	pDirInfo.m_WatchFlags			= WatchFlags;
	pDirInfo.m_UniqueId				= m_UniqueId;
	pDirInfo.m_DirectoryAltered		= DirectoryAltered;
	pDirInfo.m_pData				= pData;
	pDirInfo.m_bWatchSubTree		= bWatchSubTree;
	pDirInfo.m_bMarkedForDeletion	= FALSE;

	/* This suppresses crtical error message boxes, such as the one
	that mey arise from CreateFile() when opening attempting to
	open a floppy drive that doesn't have a floppy disk (also
	CD/DVD drives etc). */
	SetErrorMode(SEM_FAILCRITICALERRORS);

	StringCchCopy(pDirInfo.m_DirPath,MAX_PATH,Directory);

	pDirInfo.m_hDirectory = CreateFile(pDirInfo.m_DirPath,
	FILE_LIST_DIRECTORY,FILE_SHARE_READ|FILE_SHARE_DELETE|FILE_SHARE_WRITE,
	NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED,NULL);

	if(pDirInfo.m_hDirectory == INVALID_HANDLE_VALUE)
	{
		free(pData);
		return -1;
	}

	EnterCriticalSection(&m_cs);

	/* TODO: This operation alters the list, including the
	previously inserted item (e.g by updating list pointers).
	Therefore, need to lock access between this function and
	other reads of the inserted item. */
	m_DirWatchInfoList.push_back(pDirInfo);

	m_DirWatchInfoList.back().m_Async.hEvent = &m_DirWatchInfoList.back();
	QueueUserAPC(WatchDirectoryInternal,m_hThread,(ULONG_PTR)&m_DirWatchInfoList.back());

	LeaveCriticalSection(&m_cs);

	return m_UniqueId++;
}

int CDirectoryMonitor::WatchDirectory(HANDLE hDirectory,TCHAR *Directory,
UINT WatchFlags,void (*DirectoryAltered)(TCHAR *szFileName,DWORD dwAction,void *pData),
BOOL bWatchSubTree,void *pData)
{
	CDirInfo	pDirInfo;

	if(Directory == NULL)
		return -1;

	pDirInfo.m_pDirectoryMonitor	= this;
	pDirInfo.m_hThread				= m_hThread;
	pDirInfo.m_WatchFlags			= WatchFlags;
	pDirInfo.m_UniqueId				= m_UniqueId;
	pDirInfo.m_DirectoryAltered		= DirectoryAltered;
	pDirInfo.m_pData				= pData;
	pDirInfo.m_bWatchSubTree		= bWatchSubTree;
	pDirInfo.m_bMarkedForDeletion	= FALSE;

	/* This suppresses crtical error message boxes, such as the one
	that mey arise from CreateFile() when opening attempting to
	open a floppy drive that doesn't have a floppy disk (also
	CD/DVD drives etc). */
	SetErrorMode(SEM_FAILCRITICALERRORS);

	StringCchCopy(pDirInfo.m_DirPath,MAX_PATH,Directory);

	pDirInfo.m_hDirectory = hDirectory;

	EnterCriticalSection(&m_cs);

	m_DirWatchInfoList.push_back(pDirInfo);
	m_DirWatchInfoList.back().m_Async.hEvent = &m_DirWatchInfoList.back();
	//QueueUserAPC(WatchAndCreateDirectoryInternal,m_hThread,(ULONG_PTR)&m_DirWatchInfoList.back());
	QueueUserAPC(WatchDirectoryInternal,m_hThread,(ULONG_PTR)&m_DirWatchInfoList.back());

	LeaveCriticalSection(&m_cs);

	return m_UniqueId++;
}

/* These functions (CreateFile and ReadDirectoryChangesW) DO NOT appear
to put the thread in an alertable wait state. */
void CALLBACK CDirectoryMonitor::WatchAndCreateDirectoryInternal(ULONG_PTR dwParam)
{
	CDirInfo	*pDirInfo = NULL;

	pDirInfo = reinterpret_cast<CDirInfo *>(dwParam);

	pDirInfo->m_hDirectory = CreateFile(pDirInfo->m_DirPath,
	FILE_LIST_DIRECTORY,FILE_SHARE_READ|FILE_SHARE_DELETE|FILE_SHARE_WRITE,
	NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED,NULL);

	WatchDirectoryInternal(dwParam);
}

void CDirectoryMonitor::WatchDirectoryInternal(ULONG_PTR dwParam)
{
	CDirInfo	*pDirInfo = NULL;

	pDirInfo = reinterpret_cast<CDirInfo *>(dwParam);

	if(pDirInfo->m_hDirectory == INVALID_HANDLE_VALUE)
	{
		free(pDirInfo->m_pData);

		return;
	}

	pDirInfo->m_FileNotifyBuffer = (FILE_NOTIFY_INFORMATION *)malloc(PRIMARY_BUFFER_SIZE);

	pDirInfo->m_bDirMonitored = ReadDirectoryChangesW(pDirInfo->m_hDirectory,
	pDirInfo->m_FileNotifyBuffer,PRIMARY_BUFFER_SIZE,
	pDirInfo->m_bWatchSubTree,pDirInfo->m_WatchFlags,NULL,&pDirInfo->m_Async,
	CompletionRoutine);

	if(!pDirInfo->m_bDirMonitored)
	{
		free(pDirInfo->m_FileNotifyBuffer);
		CancelIo(pDirInfo->m_hDirectory);
		CloseHandle(pDirInfo->m_hDirectory);
	}
}

void CALLBACK CDirectoryMonitor::CompletionRoutine(DWORD dwErrorCode,
DWORD NumberOfBytesTransferred,LPOVERLAPPED lpOverlapped)
{
	CDirInfo				*pDirInfo = NULL;
	FILE_NOTIFY_INFORMATION	*pfni = NULL;
	TCHAR					szFileName[MAX_PATH];

	if((dwErrorCode == ERROR_SUCCESS) && (NumberOfBytesTransferred != 0))
	{
		if(lpOverlapped->hEvent == NULL)
			return;

		pDirInfo = reinterpret_cast<CDirInfo *>(lpOverlapped->hEvent);

		pfni = pDirInfo->m_FileNotifyBuffer;

		CopyDirectoryChangeFileName(pfni,szFileName,SIZEOF_ARRAY(szFileName));

		pDirInfo->m_DirectoryAltered(szFileName,pfni->Action,pDirInfo->m_pData);

		while(pfni->NextEntryOffset != 0)
		{
			pfni = (FILE_NOTIFY_INFORMATION *)((LPBYTE)pfni + pfni->NextEntryOffset);

			CopyDirectoryChangeFileName(pfni,szFileName,SIZEOF_ARRAY(szFileName));

			pDirInfo->m_DirectoryAltered(szFileName,pfni->Action,pDirInfo->m_pData);
		}

		free(pDirInfo->m_FileNotifyBuffer);

		pDirInfo->m_FileNotifyBuffer = NULL;

		/* Rewatch the directory. */
		WatchDirectoryInternal((ULONG_PTR)pDirInfo);
	}
	else if(dwErrorCode == ERROR_OPERATION_ABORTED)
	{
		pDirInfo = reinterpret_cast<CDirInfo *>(lpOverlapped->hEvent);

		/* The specified directory has stop been watched.
		Remove its entry from the queue. */
		DeleteRequest((ULONG_PTR)pDirInfo);
	}
}

void CDirectoryMonitor::DeleteRequest(ULONG_PTR dwParam)
{
	CDirInfo					*pDirInfo = NULL;
	CDirectoryMonitor			*pDirectoryMonitor = NULL;
	list<CDirInfo>::iterator	itr;

	pDirInfo = reinterpret_cast<CDirInfo *>(dwParam);

	pDirectoryMonitor = pDirInfo->m_pDirectoryMonitor;

	free(pDirInfo->m_FileNotifyBuffer);
	free(pDirInfo->m_pData);

	EnterCriticalSection(&pDirectoryMonitor->m_cs);

	for(itr = pDirectoryMonitor->m_DirWatchInfoList.begin();itr != pDirectoryMonitor->m_DirWatchInfoList.end();itr++)
	{
		if(itr->m_UniqueId == pDirInfo->m_UniqueId)
		{
			pDirectoryMonitor->m_DirWatchInfoList.erase(itr);
			break;
		}
	}

	LeaveCriticalSection(&pDirectoryMonitor->m_cs);
}

BOOL CDirectoryMonitor::StopDirectoryMonitor(int iStopId)
{
	list<CDirInfo>::iterator	itr;

	if(iStopId < 0)
		return FALSE;

	EnterCriticalSection(&m_cs);

	for(itr = m_DirWatchInfoList.begin();itr != m_DirWatchInfoList.end();itr++)
	{
		if(itr->m_UniqueId == iStopId)
		{
			/* Only stop monitoring the directory if it was
			actually monitored in the first place! */
			if(itr->m_bDirMonitored)
				QueueUserAPC(StopDirectoryWatch,m_hThread,(ULONG_PTR)itr->m_hDirectory);

			break;
		}
	}

	LeaveCriticalSection(&m_cs);

	return TRUE;
}

void CALLBACK CDirectoryMonitor::StopDirectoryWatch(ULONG_PTR dwParam)
{
	HANDLE	hDirectory;

	hDirectory = reinterpret_cast<HANDLE>(dwParam);

	CancelIo(hDirectory);
	CloseHandle(hDirectory);

	hDirectory = NULL;
}

void CDirectoryMonitor::CopyDirectoryChangeFileName(FILE_NOTIFY_INFORMATION *pfni,
TCHAR *szFileName,size_t iBufLen)
{
	#ifndef UNICODE
	nConverted = WideCharToMultiByte(CP_ACP,0,pfni->FileName,
		(pfni->FileNameLength / sizeof(pfni->FileName[0])) + 1,
		szFileName,iBufLen,NULL,NULL);

	szFileName[nConverted - 1] = '\0';
	#else
	/* FileNameLength is size in bytes NOT characters. */
	StringCchCopyN(szFileName,iBufLen,
		pfni->FileName,pfni->FileNameLength / sizeof(TCHAR));
	#endif
}