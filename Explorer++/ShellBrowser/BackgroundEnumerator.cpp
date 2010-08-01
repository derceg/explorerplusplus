#include "stdafx.h"
#include <list>
#include "BackgroundEnumerator.h"
#include "../Helper/Helper.h"

using namespace std;


DWORD WINAPI Thread_Enumerator(LPVOID pParam);

struct THREAD_INFO
{
	BackgroundEnumerator	*pbe;
	LPITEMIDLIST			pidlDirectory;
};

BackgroundEnumerator::BackgroundEnumerator(IEnumerateCallback *pEnumerateCallback)
{
	m_pEnumerateCallback = pEnumerateCallback;

	InitializeCriticalSection(&m_csStop);

	m_bStopEnumeration = FALSE;
}

BackgroundEnumerator::~BackgroundEnumerator()
{
	DeleteCriticalSection(&m_csStop);
}

/* Enumerate the specified directory in a background thread. */
HRESULT BackgroundEnumerator::EnumerateDirectory(LPITEMIDLIST pidlDirectory)
{
	THREAD_INFO *pti = new THREAD_INFO;

	pti->pbe = this;
	pti->pidlDirectory = ILClone(pidlDirectory);

	/* Each time we're asked to enumerate a directory, we'll
	spawn a new thread. */
	HANDLE hThread = CreateThread(NULL,0,Thread_Enumerator,(LPVOID)pti,
		CREATE_SUSPENDED,NULL);

	if(hThread == NULL)
	{
		return E_FAIL;
	}

	/* Drop the thread priority. */
	SetThreadPriority(hThread,THREAD_PRIORITY_BELOW_NORMAL);

	m_ThreadList.push_back(hThread);

	ResumeThread(hThread);

	return S_OK;
}

/* Enumerates a directory. */
DWORD WINAPI Thread_Enumerator(LPVOID pParam)
{
	THREAD_INFO *pti = static_cast<THREAD_INFO *>(pParam);

	std::list<LPITEMIDLIST>	*pItemList = pti->pbe->EnumerateDirectoryInternal(pti->pidlDirectory);
	pti->pbe->EnumerateDirectoryFinished(pItemList);

	CoTaskMemFree(pti->pidlDirectory);
	delete pti;

	return 0;
}

std::list<LPITEMIDLIST> *BackgroundEnumerator::EnumerateDirectoryInternal(LPITEMIDLIST pidlDirectory)
{
	std::list<LPITEMIDLIST>	*pItemList = new std::list<LPITEMIDLIST>;
	IShellFolder	*pDesktopFolder = NULL;
	IShellFolder	*pShellFolder = NULL;
	IEnumIDList		*pEnumIDList = NULL;
	LPITEMIDLIST	rgelt = NULL;
	SHCONTF			EnumFlags;
	ULONG			uFetched;
	HRESULT			hr;
	int				nItems = 0;

	hr = SHGetDesktopFolder(&pDesktopFolder);

	if(SUCCEEDED(hr))
	{
		if(IsNamespaceRoot(pidlDirectory))
		{
			hr = SHGetDesktopFolder(&pShellFolder);
		}
		else
		{
			hr = pDesktopFolder->BindToObject(pidlDirectory,NULL,
			IID_IShellFolder,(LPVOID *)&pShellFolder);
		}

		if(SUCCEEDED(hr))
		{
			EnumFlags = SHCONTF_FOLDERS|SHCONTF_NONFOLDERS|SHCONTF_INCLUDEHIDDEN;

			hr = pShellFolder->EnumObjects(NULL,EnumFlags,&pEnumIDList);

			if(SUCCEEDED(hr) && pEnumIDList != NULL)
			{
				uFetched = 1;
				while(pEnumIDList->Next(1,&rgelt,&uFetched) == S_OK && (uFetched == 1))
				{
					/* Add this item to the list. */
					pItemList->push_back(ILClone(rgelt));

					CoTaskMemFree((LPVOID)rgelt);

					/* Check if we're meant to stop... */
					/*EnterCriticalSection(&m_csStop);
					m_bStopEnumeration = TRUE;
					LeaveCriticalSection(&m_csStop);*/
				}

				pEnumIDList->Release();
			}

			pShellFolder->Release();
		}

		pDesktopFolder->Release();
	}

	return pItemList;
}

void BackgroundEnumerator::EnumerateDirectoryFinished(std::list<LPITEMIDLIST> *pItemList)
{
	std::list<LPITEMIDLIST>	*pItemListCopy = new std::list<LPITEMIDLIST>(*pItemList);

	m_pEnumerateCallback->BackgroundEnumerationFinished(*pItemList);

	/* Destroy the current list. Note that this is a
	non-standard Visual C++ construct. */
	for each(auto pidl in *pItemList)
	{
		CoTaskMemFree(pidl);
	}

	pItemList->clear();
	delete pItemList;
}

/* Stop any current enumerations. */
void BackgroundEnumerator::StopEnumeration(void)
{
	EnterCriticalSection(&m_csStop);
	m_bStopEnumeration = TRUE;
	LeaveCriticalSection(&m_csStop);
}