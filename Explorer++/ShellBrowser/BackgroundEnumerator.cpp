#include "stdafx.h"
#include <list>
#include "BackgroundEnumerator.h"
#include "../Helper/Helper.h"

using namespace std;


DWORD WINAPI Thread_Enumerator(LPVOID pParam);

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
	m_pidlDirectory = ILClone(pidlDirectory);

	m_hThread = CreateThread(NULL,0,Thread_Enumerator,(LPVOID)this,
		CREATE_SUSPENDED,NULL);

	if(m_hThread == NULL)
	{
		return E_FAIL;
	}

	/* Drop the thread priority. */
	SetThreadPriority(m_hThread,THREAD_PRIORITY_BELOW_NORMAL);

	ResumeThread(m_hThread);

	return S_OK;
}

void BackgroundEnumerator::EnumerateDirectoryInternal(void)
{
	IShellFolder	*pDesktopFolder	= NULL;
	IShellFolder	*pShellFolder		= NULL;
	IEnumIDList		*pEnumIDList		= NULL;
	LPITEMIDLIST	rgelt				= NULL;
	SHCONTF			EnumFlags;
	ULONG			uFetched;
	HRESULT			hr;
	int				nItems = 0;

	hr = SHGetDesktopFolder(&pDesktopFolder);

	if(SUCCEEDED(hr))
	{
		if(IsNamespaceRoot(m_pidlDirectory))
		{
			hr = SHGetDesktopFolder(&pShellFolder);
		}
		else
		{
			hr = pDesktopFolder->BindToObject(m_pidlDirectory,NULL,
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
					m_ItemList.push_back(ILClone(rgelt));

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
}

void BackgroundEnumerator::EnumerateDirectoryFinished(void)
{
	m_pEnumerateCallback->BackgroundEnumerationFinished();
}

/* Stop the enumeration. */
void BackgroundEnumerator::StopEnumeration(void)
{
	EnterCriticalSection(&m_csStop);
	m_bStopEnumeration = TRUE;
	LeaveCriticalSection(&m_csStop);
}

/* Enumerates a directory. */
DWORD WINAPI Thread_Enumerator(LPVOID pParam)
{
	BackgroundEnumerator *pbe = static_cast<BackgroundEnumerator *>(pParam);

	pbe->EnumerateDirectoryInternal();
	pbe->EnumerateDirectoryFinished();

	return 0;
}