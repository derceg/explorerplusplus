/******************************************************************
 *
 * Project: Explorer++
 * File: BookmarkHelper.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides several helper functions for bookmarks.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <stack>
#include "Explorer++_internal.h"
#include "BookmarkHelper.h"


CBookmarkTreeView::CBookmarkTreeView(HWND hTreeView) :
	m_hTreeView(hTreeView),
	m_uIDCounter(0)
{
	SetWindowTheme(hTreeView,L"Explorer",NULL);

	m_himl = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);
	HBITMAP hBitmap = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(m_himl,hBitmap,NULL);
	TreeView_SetImageList(hTreeView,m_himl,TVSIL_NORMAL);
	DeleteObject(hBitmap);

	TreeView_DeleteAllItems(hTreeView);
}

CBookmarkTreeView::~CBookmarkTreeView()
{
	ImageList_Destroy(m_himl);
}

void CBookmarkTreeView::InsertFoldersIntoTreeView(CBookmarkFolder *pBookmarkFolder,
	const GUID &guidSelected,const NBookmarkHelper::setExpansion_t &setExpansion)
{
	HTREEITEM hRoot = InsertFolderIntoTreeView(NULL,pBookmarkFolder,
		guidSelected,setExpansion);

	InsertFoldersIntoTreeViewRecursive(hRoot,pBookmarkFolder,
		guidSelected,setExpansion);
}

void CBookmarkTreeView::InsertFoldersIntoTreeViewRecursive(HTREEITEM hParent,
	CBookmarkFolder *pBookmarkFolder,const GUID &guidSelected,const NBookmarkHelper::setExpansion_t &setExpansion)
{
	for(auto itr = pBookmarkFolder->begin();itr != pBookmarkFolder->end();++itr)
	{
		if(CBookmarkFolder *pBookmarkFolderChild = boost::get<CBookmarkFolder>(&(*itr)))
		{
			HTREEITEM hCurrentItem = InsertFolderIntoTreeView(hParent,
				pBookmarkFolderChild,guidSelected,setExpansion);

			if(pBookmarkFolderChild->HasChildFolder())
			{
				InsertFoldersIntoTreeViewRecursive(hCurrentItem,
					pBookmarkFolderChild,guidSelected,setExpansion);
			}
		}
	}
}

HTREEITEM CBookmarkTreeView::InsertFolderIntoTreeView(HTREEITEM hParent,
	CBookmarkFolder *pBookmarkFolder,const GUID &guidSelected,const NBookmarkHelper::setExpansion_t &setExpansion)
{
	TCHAR szText[256];
	StringCchCopy(szText,SIZEOF_ARRAY(szText),pBookmarkFolder->GetName().c_str());

	int nChildren = 0;

	if(pBookmarkFolder->HasChildFolder())
	{
		nChildren = 1;
	}

	UINT uState = 0;
	UINT uStateMask = 0;

	auto itr = setExpansion.find(pBookmarkFolder->GetGUID());

	if(itr != setExpansion.end() &&
		pBookmarkFolder->HasChildFolder())
	{
		uState		|= TVIS_EXPANDED;
		uStateMask	|= TVIS_EXPANDED;
	}

	if(IsEqualGUID(pBookmarkFolder->GetGUID(),guidSelected))
	{
		uState		|= TVIS_SELECTED;
		uStateMask	|= TVIS_SELECTED;
	}

	TVITEMEX tviex;
	tviex.mask				= TVIF_TEXT|TVIF_IMAGE|TVIF_CHILDREN|TVIF_SELECTEDIMAGE|TVIF_PARAM|TVIF_STATE;
	tviex.pszText			= szText;
	tviex.iImage			= SHELLIMAGES_NEWTAB;
	tviex.iSelectedImage	= SHELLIMAGES_NEWTAB;
	tviex.cChildren			= nChildren;
	tviex.lParam			= m_uIDCounter;
	tviex.state				= uState;
	tviex.stateMask			= uStateMask;

	TVINSERTSTRUCT tvis;
	tvis.hParent			= hParent;
	tvis.hInsertAfter		= TVI_LAST;
	tvis.itemex				= tviex;
	HTREEITEM hItem = TreeView_InsertItem(m_hTreeView,&tvis);

	m_mapID.insert(std::make_pair<UINT,GUID>(m_uIDCounter,pBookmarkFolder->GetGUID()));
	++m_uIDCounter;

	return hItem;
}

CBookmarkFolder *CBookmarkTreeView::GetBookmarkFolderFromTreeView(HTREEITEM hItem,
	CBookmarkFolder *pRootBookmarkFolder)
{
	TVITEM tvi;
	tvi.mask	= TVIF_HANDLE|TVIF_PARAM;
	tvi.hItem	= hItem;
	TreeView_GetItem(m_hTreeView,&tvi);

	std::stack<UINT> stackIDs;
	HTREEITEM hParent;
	HTREEITEM hCurrentItem = hItem;

	while((hParent = TreeView_GetParent(m_hTreeView,hCurrentItem)) != NULL)
	{
		TVITEM tvi;
		tvi.mask	= TVIF_HANDLE|TVIF_PARAM;
		tvi.hItem	= hCurrentItem;
		TreeView_GetItem(m_hTreeView,&tvi);

		stackIDs.push(static_cast<UINT>(tvi.lParam));

		hCurrentItem = hParent;
	}

	CBookmarkFolder *pBookmarkFolder = pRootBookmarkFolder;

	while(!stackIDs.empty())
	{
		UINT uID = stackIDs.top();
		auto itr = m_mapID.find(uID);
		std::pair<void *,NBookmarks::BookmarkType_t> BookmarkItem = pBookmarkFolder->GetBookmarkItem(itr->second);
		pBookmarkFolder = reinterpret_cast<CBookmarkFolder *>(BookmarkItem.first);
		assert(pBookmarkFolder != NULL);

		stackIDs.pop();
	}

	return pBookmarkFolder;
}

CBookmarkListView::CBookmarkListView(HWND hListView) :
m_hListView(hListView),
m_uIDCounter(0)
{
	SetWindowTheme(hListView,L"Explorer",NULL);
	ListView_SetExtendedListViewStyleEx(hListView,
		LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT,
		LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT);

	m_himl = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);
	HBITMAP hBitmap = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(m_himl,hBitmap,NULL);
	ListView_SetImageList(hListView,m_himl,LVSIL_SMALL);
	DeleteObject(hBitmap);
}

CBookmarkListView::~CBookmarkListView()
{
	ImageList_Destroy(m_himl);
}

void CBookmarkListView::InsertBookmarksIntoListView(CBookmarkFolder *pBookmarkFolder)
{
	m_pParentBookmarkFolder = pBookmarkFolder;

	ListView_DeleteAllItems(m_hListView);
	m_uIDCounter = 0;
	m_mapID.clear();

	int iItem = 0;

	for(auto itr = pBookmarkFolder->begin();itr != pBookmarkFolder->end();++itr)
	{
		if(CBookmarkFolder *pBookmarkFolder = boost::get<CBookmarkFolder>(&(*itr)))
		{
			InsertBookmarkFolderIntoListView(pBookmarkFolder,iItem);
		}
		else if(CBookmark *pBookmark = boost::get<CBookmark>(&(*itr)))
		{
			InsertBookmarkIntoListView(pBookmark,iItem);
		}

		++iItem;
	}
}

void CBookmarkListView::InsertBookmarkFolderIntoListView(CBookmarkFolder *pBookmarkFolder,int iPosition)
{
	InsertBookmarkItemIntoListView(pBookmarkFolder->GetName(),
		pBookmarkFolder->GetGUID(),iPosition);
}

void CBookmarkListView::InsertBookmarkIntoListView(CBookmark *pBookmark,int iPosition)
{
	InsertBookmarkItemIntoListView(pBookmark->GetName(),
		pBookmark->GetGUID(),iPosition);
}

void CBookmarkListView::InsertBookmarkItemIntoListView(const std::wstring &strName,
	const GUID &guid,int iPosition)
{
	TCHAR szName[256];
	StringCchCopy(szName,SIZEOF_ARRAY(szName),strName.c_str());

	LVITEM lvi;
	lvi.mask		= LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
	lvi.iItem		= iPosition;
	lvi.iSubItem	= 0;
	lvi.iImage		= SHELLIMAGES_NEWTAB;
	lvi.pszText		= szName;
	lvi.lParam		= m_uIDCounter;
	ListView_InsertItem(m_hListView,&lvi);

	m_mapID.insert(std::make_pair<UINT,GUID>(m_uIDCounter,guid));
	++m_uIDCounter;
}

std::pair<void *,NBookmarks::BookmarkType_t> CBookmarkListView::GetBookmarkItemFromListView(int iItem)
{
	LVITEM lvi;
	lvi.mask		= LVIF_PARAM;
	lvi.iItem		= iItem;
	lvi.iSubItem	= 0;
	ListView_GetItem(m_hListView,&lvi);

	auto itr = m_mapID.find(static_cast<UINT>(lvi.lParam));
	std::pair<void *,NBookmarks::BookmarkType_t> BookmarkItem = m_pParentBookmarkFolder->GetBookmarkItem(itr->second);

	return BookmarkItem;
}