#ifndef BOOKMARKHELPER_INCLUDED
#define BOOKMARKHELPER_INCLUDED

#include "../Helper/Bookmark.h"

namespace NBookmarkHelper
{
	void			InsertFoldersIntoTreeView(HWND hTreeView,BookmarkFolder *pBookmarkFolder);
	HTREEITEM		InsertFolderIntoTreeView(HWND hTreeView,HTREEITEM hParent,BookmarkFolder *pBookmarkFolder);
	BookmarkFolder	*GetBookmarkFolderFromTreeView(HWND hTreeView,HTREEITEM hItem,BookmarkFolder *pRootBookmarkFolder);

	void			InsertBookmarksIntoListView(HWND hListView,BookmarkFolder *pBookmarkFolder);
	void			InsertBookmarkFolderIntoListView(HWND hListView,BookmarkFolder *pBookmarkFolder,int iPosition);
	void			InsertBookmarkIntoListView(HWND hListView,Bookmark *pBookmark,int iPosition);
	void			InsertBookmarkItemIntoListView(HWND hListView,const std::wstring &strName,UINT uID,int iPosition);
}

#endif