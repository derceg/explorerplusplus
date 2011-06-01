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


void InsertFoldersIntoTreeViewRecursive(HWND hTreeView,HTREEITEM hParent,
	CBookmarkFolder *pBookmarkFolder,const GUID &guidSelected,const std::unordered_set<GUID,GuidHash,GuidEq> &setExpansion);

void NBookmarkHelper::InsertFoldersIntoTreeView(HWND hTreeView,CBookmarkFolder *pBookmarkFolder,
	const GUID &guidSelected,const std::unordered_set<GUID,GuidHash,GuidEq> &setExpansion)
{
	TreeView_DeleteAllItems(hTreeView);

	HTREEITEM hRoot = InsertFolderIntoTreeView(hTreeView,NULL,pBookmarkFolder,
		guidSelected,setExpansion);

	InsertFoldersIntoTreeViewRecursive(hTreeView,hRoot,pBookmarkFolder,
		guidSelected,setExpansion);
}

void InsertFoldersIntoTreeViewRecursive(HWND hTreeView,HTREEITEM hParent,
	CBookmarkFolder *pBookmarkFolder,const GUID &guidSelected,const std::unordered_set<GUID,GuidHash,GuidEq> &setExpansion)
{
	for(auto itr = pBookmarkFolder->begin();itr != pBookmarkFolder->end();++itr)
	{
		if(CBookmarkFolder *pBookmarkFolderChild = boost::get<CBookmarkFolder>(&(*itr)))
		{
			HTREEITEM hCurrentItem = NBookmarkHelper::InsertFolderIntoTreeView(hTreeView,hParent,
				pBookmarkFolderChild,guidSelected,setExpansion);

			if(pBookmarkFolderChild->HasChildFolder())
			{
				InsertFoldersIntoTreeViewRecursive(hTreeView,hCurrentItem,
					pBookmarkFolderChild,guidSelected,setExpansion);
			}
		}
	}
}

/* Note that this function assumes that the standard shell images
have been placed in the image list for the treeview. */
HTREEITEM NBookmarkHelper::InsertFolderIntoTreeView(HWND hTreeView,HTREEITEM hParent,
	CBookmarkFolder *pBookmarkFolder,const GUID &guidSelected,const std::unordered_set<GUID,GuidHash,GuidEq> &setExpansion)
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
	tviex.lParam			= reinterpret_cast<LPARAM>(pBookmarkFolder);
	tviex.state				= uState;
	tviex.stateMask			= uStateMask;

	TVINSERTSTRUCT tvis;
	tvis.hParent			= hParent;
	tvis.hInsertAfter		= TVI_LAST;
	tvis.itemex				= tviex;
	HTREEITEM hItem = TreeView_InsertItem(hTreeView,&tvis);

	return hItem;
}

CBookmarkFolder *NBookmarkHelper::GetBookmarkFolderFromTreeView(HWND hTreeView,
	HTREEITEM hItem,CBookmarkFolder *pRootBookmarkFolder)
{
	TVITEM tvi;
	tvi.mask	= TVIF_HANDLE|TVIF_PARAM;
	tvi.hItem	= hItem;
	TreeView_GetItem(hTreeView,&tvi);

	CBookmarkFolder *pBookmarkFolder = reinterpret_cast<CBookmarkFolder *>(tvi.lParam);

	return pBookmarkFolder;

	/*std::stack<UINT> stackIDs;
	HTREEITEM hParent;
	HTREEITEM hCurrentItem = hItem;

	while((hParent = TreeView_GetParent(hTreeView,hCurrentItem)) != NULL)
	{
		TVITEM tvi;
		tvi.mask	= TVIF_HANDLE|TVIF_PARAM;
		tvi.hItem	= hCurrentItem;
		TreeView_GetItem(hTreeView,&tvi);

		stackIDs.push(static_cast<UINT>(tvi.lParam));

		hCurrentItem = hParent;
	}

	CBookmarkFolder *pBookmarkFolder = pRootBookmarkFolder;

	while(!stackIDs.empty())
	{
		UINT uID = stackIDs.top();
		std::pair<void *,NBookmarks::BookmarkType_t> BookmarkItem = pBookmarkFolder->GetBookmarkItem(uID);
		pBookmarkFolder = reinterpret_cast<CBookmarkFolder *>(BookmarkItem.first);
		assert(pBookmarkFolder != NULL);

		stackIDs.pop();
	}

	return pBookmarkFolder;*/
}

void NBookmarkHelper::InsertBookmarksIntoListView(HWND hListView,CBookmarkFolder *pBookmarkFolder)
{
	ListView_DeleteAllItems(hListView);

	int iItem = 0;

	for(auto itr = pBookmarkFolder->begin();itr != pBookmarkFolder->end();++itr)
	{
		if(CBookmarkFolder *pBookmarkFolder = boost::get<CBookmarkFolder>(&(*itr)))
		{
			InsertBookmarkFolderIntoListView(hListView,pBookmarkFolder,iItem);
		}
		else if(CBookmark *pBookmark = boost::get<CBookmark>(&(*itr)))
		{
			InsertBookmarkIntoListView(hListView,pBookmark,iItem);
		}

		iItem++;
	}
}

/* TODO: */
void NBookmarkHelper::InsertBookmarkFolderIntoListView(HWND hListView,
	CBookmarkFolder *pBookmarkFolder,int iPosition)
{
	InsertBookmarkItemIntoListView(hListView,pBookmarkFolder->GetName(),
		0,iPosition);
}

void NBookmarkHelper::InsertBookmarkIntoListView(HWND hListView,
	CBookmark *pBookmark,int iPosition)
{
	InsertBookmarkItemIntoListView(hListView,pBookmark->GetName(),
		0,iPosition);
}

void NBookmarkHelper::InsertBookmarkItemIntoListView(HWND hListView,
	const std::wstring &strName,UINT uID,int iPosition)
{
	TCHAR szName[256];
	StringCchCopy(szName,SIZEOF_ARRAY(szName),strName.c_str());

	LVITEM lvi;
	lvi.mask		= LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
	lvi.iItem		= iPosition;
	lvi.iSubItem	= 0;
	lvi.iImage		= 0;
	lvi.pszText		= szName;
	lvi.lParam		= uID;
	ListView_InsertItem(hListView,&lvi);
}