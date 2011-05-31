#ifndef BOOKMARKHELPER_INCLUDED
#define BOOKMARKHELPER_INCLUDED

#include <set>
#include "../Helper/Bookmark.h"

namespace NBookmarkHelper
{
	void			InsertFoldersIntoTreeView(HWND hTreeView,CBookmarkFolder *pBookmarkFolder,UINT uIDSelected,const std::set<UINT> &setExpansion);
	HTREEITEM		InsertFolderIntoTreeView(HWND hTreeView,HTREEITEM hParent,CBookmarkFolder *pBookmarkFolder,UINT uIDSelected,const std::set<UINT> &setExpansion);
	CBookmarkFolder	*GetBookmarkFolderFromTreeView(HWND hTreeView,HTREEITEM hItem,CBookmarkFolder *pRootBookmarkFolder);

	void			InsertBookmarksIntoListView(HWND hListView,CBookmarkFolder *pBookmarkFolder);
	void			InsertBookmarkFolderIntoListView(HWND hListView,CBookmarkFolder *pBookmarkFolder,int iPosition);
	void			InsertBookmarkIntoListView(HWND hListView,CBookmark *pBookmark,int iPosition);
	void			InsertBookmarkItemIntoListView(HWND hListView,const std::wstring &strName,UINT uID,int iPosition);
}

#endif