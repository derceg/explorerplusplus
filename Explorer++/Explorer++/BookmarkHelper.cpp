/******************************************************************
 *
 * Project: Explorer++
 * File: BookmarkHelper.cpp
 * License: GPL - See LICENSE in the top level directory
 *
 * Provides several helper functions for bookmarks.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <stack>
#include <algorithm>
#include "Explorer++_internal.h"
#include "MainImages.h"
#include "BookmarkHelper.h"
#include "MainResource.h"
#include "../Helper/Macros.h"


int CALLBACK SortByName(const NBookmarkHelper::variantBookmark_t BookmarkItem1,const NBookmarkHelper::variantBookmark_t BookmarkItem2);
int CALLBACK SortByLocation(const NBookmarkHelper::variantBookmark_t BookmarkItem1,const NBookmarkHelper::variantBookmark_t BookmarkItem2);
int CALLBACK SortByVisitDate(const NBookmarkHelper::variantBookmark_t BookmarkItem1,const NBookmarkHelper::variantBookmark_t BookmarkItem2);
int CALLBACK SortByVisitCount(const NBookmarkHelper::variantBookmark_t BookmarkItem1,const NBookmarkHelper::variantBookmark_t BookmarkItem2);
int CALLBACK SortByAdded(const NBookmarkHelper::variantBookmark_t BookmarkItem1,const NBookmarkHelper::variantBookmark_t BookmarkItem2);
int CALLBACK SortByLastModified(const NBookmarkHelper::variantBookmark_t BookmarkItem1,const NBookmarkHelper::variantBookmark_t BookmarkItem2);

CBookmarkTreeView::CBookmarkTreeView(HWND hTreeView,CBookmarkFolder *pAllBookmarks,
	const GUID &guidSelected,const NBookmarkHelper::setExpansion_t &setExpansion) :
	m_hTreeView(hTreeView),
	m_pAllBookmarks(pAllBookmarks),
	m_uIDCounter(0)
{
	SetWindowSubclass(hTreeView,BookmarkTreeViewProcStub,0,reinterpret_cast<DWORD_PTR>(this));

	SetWindowTheme(hTreeView,L"Explorer",NULL);

	m_himl = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);
	HBITMAP hBitmap = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(m_himl,hBitmap,NULL);
	TreeView_SetImageList(hTreeView,m_himl,TVSIL_NORMAL);
	DeleteObject(hBitmap);

	SetupTreeView(guidSelected,setExpansion);
}

CBookmarkTreeView::~CBookmarkTreeView()
{
	RemoveWindowSubclass(m_hTreeView,BookmarkTreeViewProcStub,0);
	ImageList_Destroy(m_himl);
}

LRESULT CALLBACK BookmarkTreeViewProcStub(HWND hwnd,UINT uMsg,
	WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	CBookmarkTreeView *pbtv = reinterpret_cast<CBookmarkTreeView *>(dwRefData);

	return pbtv->TreeViewProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK CBookmarkTreeView::TreeViewProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
	case WM_NOTIFY:
		switch(reinterpret_cast<NMHDR *>(lParam)->code)
		{
		case TVN_DELETEITEM:
			OnTvnDeleteItem(reinterpret_cast<NMTREEVIEW *>(lParam));
			break;
		}
		break;
	}

	return DefSubclassProc(hwnd,Msg,wParam,lParam);
}

void CBookmarkTreeView::SetupTreeView(const GUID &guidSelected,const NBookmarkHelper::setExpansion_t &setExpansion)
{
	TreeView_DeleteAllItems(m_hTreeView);

	HTREEITEM hRoot = InsertFolderIntoTreeView(NULL,*m_pAllBookmarks,0);
	InsertFoldersIntoTreeViewRecursive(hRoot,*m_pAllBookmarks);

	for each(auto guidExpanded in setExpansion)
	{
		auto itrExpanded = m_mapItem.find(guidExpanded);

		if(itrExpanded != m_mapItem.end())
		{
			CBookmarkFolder &BookmarkFolder = GetBookmarkFolderFromTreeView(itrExpanded->second);

			if(BookmarkFolder.HasChildFolder())
			{
				TreeView_Expand(m_hTreeView,itrExpanded->second,TVE_EXPAND);
			}
		}
	}

	auto itrSelected = m_mapItem.find(guidSelected);
	
	if(itrSelected != m_mapItem.end())
	{
		TreeView_SelectItem(m_hTreeView,itrSelected->second);
	}
}

void CBookmarkTreeView::InsertFoldersIntoTreeViewRecursive(HTREEITEM hParent,const CBookmarkFolder &BookmarkFolder)
{
	std::size_t Position = 0;

	for(auto itr = BookmarkFolder.begin();itr != BookmarkFolder.end();++itr)
	{
		if(itr->type() == typeid(CBookmarkFolder))
		{
			const CBookmarkFolder &BookmarkFolderChild = boost::get<CBookmarkFolder>(*itr);

			HTREEITEM hCurrentItem = InsertFolderIntoTreeView(hParent,
				BookmarkFolderChild,Position);
			Position++;

			if(BookmarkFolderChild.HasChildFolder())
			{
				InsertFoldersIntoTreeViewRecursive(hCurrentItem,
					BookmarkFolderChild);
			}
		}
	}
}

HTREEITEM CBookmarkTreeView::InsertFolderIntoTreeView(HTREEITEM hParent,const CBookmarkFolder &BookmarkFolder,std::size_t Position)
{
	TCHAR szText[256];
	StringCchCopy(szText,SIZEOF_ARRAY(szText),BookmarkFolder.GetName().c_str());

	int nChildren = 0;

	if(BookmarkFolder.HasChildFolder())
	{
		nChildren = 1;
	}

	TVITEMEX tviex;
	tviex.mask				= TVIF_TEXT|TVIF_IMAGE|TVIF_CHILDREN|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	tviex.pszText			= szText;
	tviex.iImage			= SHELLIMAGES_NEWTAB;
	tviex.iSelectedImage	= SHELLIMAGES_NEWTAB;
	tviex.cChildren			= nChildren;
	tviex.lParam			= m_uIDCounter;

	HTREEITEM hInsertAfter;

	if(Position == 0)
	{
		hInsertAfter = TVI_FIRST;
	}
	else
	{
		/* Find the item one *before* Position;
		the new item will then be inserted one
		place *after* this. */
		HTREEITEM hChild = TreeView_GetChild(m_hTreeView,hParent);

		for(std::size_t i = 0;i < (Position - 1);i++)
		{
			HTREEITEM hNextSibling = TreeView_GetNextSibling(m_hTreeView,hChild);
			
			/* Only bookmark folders are inserted into the
			treeview, so it's possible that the specified position
			will be after the last child in the treeview. */
			if(hNextSibling == NULL)
			{
				break;
			}

			hChild = hNextSibling;
		}

		hInsertAfter = hChild;
	}

	TVINSERTSTRUCT tvis;
	tvis.hParent			= hParent;
	tvis.hInsertAfter		= hInsertAfter;
	tvis.itemex				= tviex;
	HTREEITEM hItem = TreeView_InsertItem(m_hTreeView,&tvis);

	m_mapID.insert(std::make_pair(m_uIDCounter,BookmarkFolder.GetGUID()));
	++m_uIDCounter;

	m_mapItem.insert(std::make_pair(BookmarkFolder.GetGUID(),hItem));

	return hItem;
}

HTREEITEM CBookmarkTreeView::BookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,
	const CBookmarkFolder &BookmarkFolder,std::size_t Position)
{
	/* Due to the fact that *all* bookmark folders will be inserted
	into the treeview (regardless of whether or not they are actually
	shown), any new folders will always need to be inserted. */
	auto itr = m_mapItem.find(ParentBookmarkFolder.GetGUID());
	assert(itr != m_mapItem.end());
	HTREEITEM hItem = InsertFolderIntoTreeView(itr->second,BookmarkFolder,Position);

	UINT uParentState = TreeView_GetItemState(m_hTreeView,itr->second,TVIS_EXPANDED);

	if((uParentState & TVIS_EXPANDED) != TVIS_EXPANDED)
	{
		TVITEM tvi;
		tvi.mask		= TVIF_CHILDREN;
		tvi.hItem		= itr->second;
		tvi.cChildren	= 1;
		TreeView_SetItem(m_hTreeView,&tvi);
	}

	return hItem;
}

void CBookmarkTreeView::BookmarkFolderModified(const GUID &guid)
{
	auto itr = m_mapItem.find(guid);
	assert(itr != m_mapItem.end());

	CBookmarkFolder &BookmarkFolder = GetBookmarkFolderFromTreeView(itr->second);

	TCHAR szText[256];
	StringCchCopy(szText,SIZEOF_ARRAY(szText),BookmarkFolder.GetName().c_str());

	/* The only property of the bookmark folder shown
	within the treeview is its name, so that is all
	that needs to be updated here. */
	TVITEM tvi;
	tvi.mask		= TVIF_TEXT;
	tvi.hItem		= itr->second;
	tvi.pszText		= szText;
	TreeView_SetItem(m_hTreeView,&tvi);
}

void CBookmarkTreeView::BookmarkFolderRemoved(const GUID &guid)
{
	auto itr = m_mapItem.find(guid);
	assert(itr != m_mapItem.end());

	/* TODO: Should collapse parent if it no longer
	has any children. Should also change selection if
	required (i.e. if the deleted bookmark was selected). */
	TreeView_DeleteItem(m_hTreeView,itr->second);
}

void CBookmarkTreeView::OnTvnDeleteItem(NMTREEVIEW *pnmtv)
{
	auto itrID = m_mapID.find(static_cast<UINT>(pnmtv->itemOld.lParam));

	if(itrID == m_mapID.end())
	{
		assert(false);
	}

	auto itrItem = m_mapItem.find(itrID->second);

	if(itrItem == m_mapItem.end())
	{
		assert(false);
	}

	m_mapItem.erase(itrItem);
	m_mapID.erase(itrID);
}

void CBookmarkTreeView::SelectFolder(const GUID &guid)
{
	auto itr = m_mapItem.find(guid);

	assert(itr != m_mapItem.end());

	TreeView_SelectItem(m_hTreeView,itr->second);
}

CBookmarkFolder &CBookmarkTreeView::GetBookmarkFolderFromTreeView(HTREEITEM hItem)
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
		tvi.mask	= TVIF_HANDLE|TVIF_PARAM;
		tvi.hItem	= hCurrentItem;
		TreeView_GetItem(m_hTreeView,&tvi);

		stackIDs.push(static_cast<UINT>(tvi.lParam));

		hCurrentItem = hParent;
	}

	CBookmarkFolder *pBookmarkFolder = m_pAllBookmarks;

	while(!stackIDs.empty())
	{
		UINT uID = stackIDs.top();
		auto itr = m_mapID.find(uID);

		NBookmarkHelper::variantBookmark_t variantBookmark = NBookmarkHelper::GetBookmarkItem(*pBookmarkFolder,itr->second);
		pBookmarkFolder = boost::get<CBookmarkFolder>(&variantBookmark);

		stackIDs.pop();
	}

	return *pBookmarkFolder;
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

void CBookmarkListView::InsertBookmarksIntoListView(const CBookmarkFolder &BookmarkFolder)
{
	ListView_DeleteAllItems(m_hListView);
	m_uIDCounter = 0;
	m_mapID.clear();

	int iItem = 0;

	for(auto itr = BookmarkFolder.begin();itr != BookmarkFolder.end();++itr)
	{
		if(itr->type() == typeid(CBookmarkFolder))
		{
			const CBookmarkFolder &CurrentBookmarkFolder = boost::get<CBookmarkFolder>(*itr);
			InsertBookmarkFolderIntoListView(CurrentBookmarkFolder,iItem);
		}
		else
		{
			const CBookmark &CurrentBookmark = boost::get<CBookmark>(*itr);
			InsertBookmarkIntoListView(CurrentBookmark,iItem);
		}

		++iItem;
	}
}

int CBookmarkListView::InsertBookmarkFolderIntoListView(const CBookmarkFolder &BookmarkFolder,int iPosition)
{
	return InsertBookmarkItemIntoListView(BookmarkFolder.GetName(),
		BookmarkFolder.GetGUID(),true,iPosition);
}

int CBookmarkListView::InsertBookmarkIntoListView(const CBookmark &Bookmark,int iPosition)
{
	return InsertBookmarkItemIntoListView(Bookmark.GetName(),
		Bookmark.GetGUID(),false,iPosition);
}

int CBookmarkListView::InsertBookmarkItemIntoListView(const std::wstring &strName,
	const GUID &guid,bool bFolder,int iPosition)
{
	assert(iPosition >= 0 && iPosition <= ListView_GetItemCount(m_hListView));

	TCHAR szName[256];
	StringCchCopy(szName,SIZEOF_ARRAY(szName),strName.c_str());

	int iImage;

	if(bFolder)
	{
		iImage = SHELLIMAGES_NEWTAB;
	}
	else
	{
		iImage = SHELLIMAGES_FAV;
	}

	LVITEM lvi;
	lvi.mask		= LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
	lvi.iItem		= iPosition;
	lvi.iSubItem	= 0;
	lvi.iImage		= iImage;
	lvi.pszText		= szName;
	lvi.lParam		= m_uIDCounter;
	int iItem = ListView_InsertItem(m_hListView,&lvi);

	m_mapID.insert(std::make_pair(m_uIDCounter,guid));
	++m_uIDCounter;

	return iItem;
}

NBookmarkHelper::variantBookmark_t CBookmarkListView::GetBookmarkItemFromListView(CBookmarkFolder &ParentBookmarkFolder,int iItem)
{
	LVITEM lvi;
	lvi.mask		= LVIF_PARAM;
	lvi.iItem		= iItem;
	lvi.iSubItem	= 0;
	ListView_GetItem(m_hListView,&lvi);

	NBookmarkHelper::variantBookmark_t variantBookmark = GetBookmarkItemFromListViewlParam(ParentBookmarkFolder,lvi.lParam);

	return variantBookmark;
}

NBookmarkHelper::variantBookmark_t CBookmarkListView::GetBookmarkItemFromListViewlParam(CBookmarkFolder &ParentBookmarkFolder,LPARAM lParam)
{
	auto itr = m_mapID.find(static_cast<UINT>(lParam));
	NBookmarkHelper::variantBookmark_t variantBookmark = NBookmarkHelper::GetBookmarkItem(ParentBookmarkFolder,itr->second);

	return variantBookmark;
}

NBookmarkHelper::variantBookmark_t NBookmarkHelper::GetBookmarkItem(CBookmarkFolder &ParentBookmarkFolder,
	const GUID &guid)
{
	auto itr = std::find_if(ParentBookmarkFolder.begin(),ParentBookmarkFolder.end(),
		[guid](boost::variant<CBookmarkFolder,CBookmark> &variantBookmark) -> BOOL
		{
			if(variantBookmark.type() == typeid(CBookmarkFolder))
			{
				CBookmarkFolder BookmarkFolder = boost::get<CBookmarkFolder>(variantBookmark);
				return IsEqualGUID(BookmarkFolder.GetGUID(),guid);
			}
			else
			{
				CBookmark Bookmark = boost::get<CBookmark>(variantBookmark);
				return IsEqualGUID(Bookmark.GetGUID(),guid);
			}
		}
	);

	assert(itr != ParentBookmarkFolder.end());

	if(itr->type() == typeid(CBookmarkFolder))
	{
		CBookmarkFolder &BookmarkFolder = boost::get<CBookmarkFolder>(*itr);
		return BookmarkFolder;
	}
	else
	{
		CBookmark &Bookmark = boost::get<CBookmark>(*itr);
		return Bookmark;
	}
}

int CALLBACK NBookmarkHelper::Sort(SortMode_t SortMode,const variantBookmark_t BookmarkItem1,
	const variantBookmark_t BookmarkItem2)
{
	if(BookmarkItem1.type() == typeid(CBookmarkFolder) &&
		BookmarkItem2.type() == typeid(CBookmark))
	{
		return -1;
	}
	else if(BookmarkItem1.type() == typeid(CBookmark) &&
		BookmarkItem2.type() == typeid(CBookmarkFolder))
	{
		return 1;
	}
	else
	{
		int iRes = 0;

		switch(SortMode)
		{
		case SM_NAME:
			iRes = SortByName(BookmarkItem1,BookmarkItem2);
			break;

		case SM_LOCATION:
			iRes = SortByLocation(BookmarkItem1,BookmarkItem2);
			break;

		case SM_VISIT_DATE:
			iRes = SortByVisitDate(BookmarkItem1,BookmarkItem2);
			break;

		case SM_VISIT_COUNT:
			iRes = SortByVisitCount(BookmarkItem1,BookmarkItem2);
			break;

		case SM_ADDED:
			iRes = SortByAdded(BookmarkItem1,BookmarkItem2);
			break;

		case SM_LAST_MODIFIED:
			iRes = SortByLastModified(BookmarkItem1,BookmarkItem2);
			break;

		default:
			assert(false);
			break;
		}

		return iRes;
	}
}

int CALLBACK SortByName(const NBookmarkHelper::variantBookmark_t BookmarkItem1,
	const NBookmarkHelper::variantBookmark_t BookmarkItem2)
{
	if(BookmarkItem1.type() == typeid(CBookmarkFolder) &&
		BookmarkItem2.type() == typeid(CBookmarkFolder))
	{
		const CBookmarkFolder &BookmarkFolder1 = boost::get<CBookmarkFolder>(BookmarkItem1);
		const CBookmarkFolder &BookmarkFolder2 = boost::get<CBookmarkFolder>(BookmarkItem2);

		return BookmarkFolder1.GetName().compare(BookmarkFolder2.GetName());
	}
	else
	{
		const CBookmark &Bookmark1 = boost::get<CBookmark>(BookmarkItem1);
		const CBookmark &Bookmark2 = boost::get<CBookmark>(BookmarkItem1);

		return Bookmark1.GetName().compare(Bookmark2.GetName());
	}
}

int CALLBACK SortByLocation(const NBookmarkHelper::variantBookmark_t BookmarkItem1,
	const NBookmarkHelper::variantBookmark_t BookmarkItem2)
{
	if(BookmarkItem1.type() == typeid(CBookmarkFolder) &&
		BookmarkItem2.type() == typeid(CBookmarkFolder))
	{
		return 0;
	}
	else
	{
		const CBookmark &Bookmark1 = boost::get<CBookmark>(BookmarkItem1);
		const CBookmark &Bookmark2 = boost::get<CBookmark>(BookmarkItem1);

		return Bookmark1.GetLocation().compare(Bookmark2.GetLocation());
	}
}

int CALLBACK SortByVisitDate(const NBookmarkHelper::variantBookmark_t BookmarkItem1,
	const NBookmarkHelper::variantBookmark_t BookmarkItem2)
{
	if(BookmarkItem1.type() == typeid(CBookmarkFolder) &&
		BookmarkItem2.type() == typeid(CBookmarkFolder))
	{
		return 0;
	}
	else
	{
		const CBookmark &Bookmark1 = boost::get<CBookmark>(BookmarkItem1);
		const CBookmark &Bookmark2 = boost::get<CBookmark>(BookmarkItem1);

		FILETIME ft1 = Bookmark1.GetDateLastVisited();
		FILETIME ft2 = Bookmark2.GetDateLastVisited();

		return CompareFileTime(&ft1,&ft2);
	}
}

int CALLBACK SortByVisitCount(const NBookmarkHelper::variantBookmark_t BookmarkItem1,
	const NBookmarkHelper::variantBookmark_t BookmarkItem2)
{
	if(BookmarkItem1.type() == typeid(CBookmarkFolder) &&
		BookmarkItem2.type() == typeid(CBookmarkFolder))
	{
		return 0;
	}
	else
	{
		const CBookmark &Bookmark1 = boost::get<CBookmark>(BookmarkItem1);
		const CBookmark &Bookmark2 = boost::get<CBookmark>(BookmarkItem1);

		return Bookmark1.GetVisitCount() - Bookmark2.GetVisitCount();
	}
}

int CALLBACK SortByAdded(const NBookmarkHelper::variantBookmark_t BookmarkItem1,
	const NBookmarkHelper::variantBookmark_t BookmarkItem2)
{
	if(BookmarkItem1.type() == typeid(CBookmarkFolder) &&
		BookmarkItem2.type() == typeid(CBookmarkFolder))
	{
		const CBookmarkFolder &BookmarkFolder1 = boost::get<CBookmarkFolder>(BookmarkItem1);
		const CBookmarkFolder &BookmarkFolder2 = boost::get<CBookmarkFolder>(BookmarkItem2);

		FILETIME ft1 = BookmarkFolder1.GetDateCreated();
		FILETIME ft2 = BookmarkFolder2.GetDateCreated();

		return CompareFileTime(&ft1,&ft2);
	}
	else
	{
		const CBookmark &Bookmark1 = boost::get<CBookmark>(BookmarkItem1);
		const CBookmark &Bookmark2 = boost::get<CBookmark>(BookmarkItem1);

		FILETIME ft1 = Bookmark1.GetDateCreated();
		FILETIME ft2 = Bookmark2.GetDateCreated();

		return CompareFileTime(&ft1,&ft2);
	}
}

int CALLBACK SortByLastModified(const NBookmarkHelper::variantBookmark_t BookmarkItem1,
	const NBookmarkHelper::variantBookmark_t BookmarkItem2)
{
	if(BookmarkItem1.type() == typeid(CBookmarkFolder) &&
		BookmarkItem2.type() == typeid(CBookmarkFolder))
	{
		const CBookmarkFolder &BookmarkFolder1 = boost::get<CBookmarkFolder>(BookmarkItem1);
		const CBookmarkFolder &BookmarkFolder2 = boost::get<CBookmarkFolder>(BookmarkItem2);

		FILETIME ft1 = BookmarkFolder1.GetDateModified();
		FILETIME ft2 = BookmarkFolder2.GetDateModified();

		return CompareFileTime(&ft1,&ft2);
	}
	else
	{
		const CBookmark &Bookmark1 = boost::get<CBookmark>(BookmarkItem1);
		const CBookmark &Bookmark2 = boost::get<CBookmark>(BookmarkItem1);

		FILETIME ft1 = Bookmark1.GetDateModified();
		FILETIME ft2 = Bookmark2.GetDateModified();

		return CompareFileTime(&ft1,&ft2);
	}
}