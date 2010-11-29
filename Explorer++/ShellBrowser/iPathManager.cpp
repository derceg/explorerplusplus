/******************************************************************
 *
 * Project: ShellBrowser
 * File: iPathManager.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Remembers path history, and
 * includes the ability to browse
 * back/forward through a set
 * of paths.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "iPathManager.h"
#include "../Helper/Helper.h"
#include "../Helper/ShellHelper.h"


using namespace std;

CPathManager::CPathManager()
{
	m_iRefCount = 1;

	m_iCurrent = 0;

	m_nAllocated = DEFAULT_ALLOCATION;
	ppidlList = (LPITEMIDLIST *)malloc(m_nAllocated * sizeof(LPITEMIDLIST));

	m_nTotal = 0;
}

CPathManager::~CPathManager()
{
}

/* IUnknown interface members. */
HRESULT __stdcall CPathManager::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = NULL;

	if(iid == IID_IUnknown)
	{
		*ppvObject = static_cast<IUnknown *>(this);
	}

	if(*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall CPathManager::AddRef(void)
{
	return ++m_iRefCount;
}

ULONG __stdcall CPathManager::Release(void)
{
	m_iRefCount--;
	
	if(m_iRefCount==0)
	{
		delete this;
		return 0;
	}

	return m_iRefCount;
}

void CPathManager::StoreIdl(LPITEMIDLIST pidl)
{
	/* Check if the number of idl's stored has reached the number of
	spaces allocated. If so, allocate a new block. */
	if(m_iCurrent >= (m_nAllocated - 1))
	{
		m_nAllocated += DEFAULT_ALLOCATION;

		ppidlList = (LPITEMIDLIST *)realloc(ppidlList,
		m_nAllocated * sizeof(LPITEMIDLIST));
	}

	ppidlList[m_iCurrent++] = ILClone(pidl);

	/* "Erases" idl's forward of the current one. */
	m_nTotal = m_iCurrent;
}

void CPathManager::ShiftIdlArray(int iStart)
{
	int i = 0;

	for(i = iStart;i < m_nTotal - 1;i++)
	{
		ppidlList[i] = ppidlList[i + 1];
	}

	m_nTotal--;
}

LPITEMIDLIST CPathManager::RetrievePath(int iIndex)
{
	if((m_iCurrent + iIndex) < 0 ||
	(m_iCurrent + iIndex) > m_nTotal)
	{
		return NULL;
	}

	/* Update the current folder pointer to point to the
	folder that was selected. */
	m_iCurrent += iIndex;

	return ILClone(ppidlList[m_iCurrent - 1]);
}

LPITEMIDLIST CPathManager::RetrievePathWithoutUpdate(int iIndex)
{
	if((m_iCurrent + iIndex) < 0 ||
	(m_iCurrent + iIndex) > m_nTotal)
	{
		return NULL;
	}

	return ILClone(ppidlList[m_iCurrent + iIndex - 1]);
}

LPITEMIDLIST CPathManager::RetrieveAndValidateIdl(int iIndex)
{
	if((m_iCurrent + iIndex) < 0 ||
	(m_iCurrent + iIndex) > m_nTotal)
	{
		return NULL;
	}

	if(!CheckIdl(ppidlList[m_iCurrent + iIndex - 1]))
	{
		CoTaskMemFree(ppidlList[m_iCurrent + iIndex - 1]);
		ShiftIdlArray(m_iCurrent + iIndex - 1);

		return NULL;
	}

	/* Update the current folder pointer to point to the
	folder that was selected. */
	m_iCurrent += iIndex;

	return ILClone(ppidlList[m_iCurrent - 1]);
}

int CPathManager::GetNumBackPathsStored(void)
{
	/* CurrentPath pointer points to the current path in the array.
	All items before this one are 'back' paths, all items after
	this one are 'forward' paths. */
	return m_iCurrent - 1;
}

int CPathManager::GetNumForwardPathsStored(void)
{
	/* iNumStoredPaths indexes the end of the array.
	Difference between it and the iCurrentPath index
	gives the number of 'forward' paths stored. */
	return m_nTotal - m_iCurrent;
}

void CPathManager::GetBackHistory(list<LPITEMIDLIST> *lHistory)
{
	int nPaths;
	int iStartIndex;
	int iEndIndex;
	int i = 0;

	nPaths = GetNumBackPathsStored();

	iEndIndex = m_iCurrent - 1;

	iStartIndex = m_iCurrent > 10 ? m_iCurrent - 10 : 0;

	for(i = iEndIndex - 1;i >= iStartIndex;i--)
	{
		lHistory->push_back(ILClone(ppidlList[i]));
	}
}

void CPathManager::GetForwardHistory(list<LPITEMIDLIST> *lHistory)
{
	int nPaths;
	int iStartIndex;
	int iEndIndex;
	int i = 0;

	nPaths = GetNumForwardPathsStored();

	iStartIndex = m_iCurrent;

	iEndIndex = (m_nTotal - m_iCurrent) > 10 ? m_iCurrent + 10 : m_nTotal;

	for(i = iStartIndex;i < iEndIndex;i++)
	{
		lHistory->push_back(ILClone(ppidlList[i]));
	}
}

UINT CPathManager::CreateHistoryPopupMenu(HWND Parent,POINT *Origin,BOOL bBack)
{
	HMENU	hMenu;
	TCHAR	szMenuText[MAX_PATH];
	UINT	ItemReturned;
	int		NumMenuPaths;
	int		iStartIndex;
	int		iEndIndex;
	int		i = 0;

	hMenu = CreatePopupMenu();

	if(bBack)
	{
		NumMenuPaths = GetNumBackPathsStored();

		iEndIndex = m_iCurrent - 1;

		iStartIndex = m_iCurrent > 10 ? m_iCurrent - 10 : 0;

		for(i = iEndIndex - 1;i >= iStartIndex;i--)
		{
			GetDisplayName(ppidlList[i],szMenuText,SHGDN_INFOLDER);
			AppendMenu(hMenu,MF_STRING,iEndIndex - i,szMenuText);
		}
	}
	else
	{
		NumMenuPaths = GetNumForwardPathsStored();

		iStartIndex = m_iCurrent;

		iEndIndex = (m_nTotal - m_iCurrent) > 10 ? m_iCurrent + 10 : m_nTotal;

		for(i = iStartIndex;i < iEndIndex;i++)
		{
			GetDisplayName(ppidlList[i],szMenuText,SHGDN_INFOLDER);
			AppendMenu(hMenu,MF_STRING,i - iStartIndex + 1,szMenuText);
		}
	}

	/* Show the popup menu. */
	ItemReturned = TrackPopupMenu(hMenu,TPM_LEFTALIGN|TPM_RETURNCMD,Origin->x,Origin->y,0,Parent,0);

	return ItemReturned;
}