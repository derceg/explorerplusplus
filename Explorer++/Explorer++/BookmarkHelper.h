#ifndef BOOKMARKHELPER_INCLUDED
#define BOOKMARKHELPER_INCLUDED

#include <unordered_set>
#include "../Helper/Bookmark.h"

struct GuidEq
{
	bool operator () (const GUID &guid1,const GUID &guid2) const
	{
		return (IsEqualGUID(guid1,guid2) == TRUE);
	}
};

struct GuidHash
{
	size_t operator () (const GUID &guid) const
	{
		return guid.Data1;
	}
};

namespace NBookmarkHelper
{
	void			InsertFoldersIntoTreeView(HWND hTreeView,CBookmarkFolder *pBookmarkFolder,const GUID &guidSelected,const std::unordered_set<GUID,GuidHash,GuidEq> &setExpansion);
	HTREEITEM		InsertFolderIntoTreeView(HWND hTreeView,HTREEITEM hParent,CBookmarkFolder *pBookmarkFolder,const GUID &guidSelected,const std::unordered_set<GUID,GuidHash,GuidEq> &setExpansion);
	CBookmarkFolder	*GetBookmarkFolderFromTreeView(HWND hTreeView,HTREEITEM hItem,CBookmarkFolder *pRootBookmarkFolder);

	void			InsertBookmarksIntoListView(HWND hListView,CBookmarkFolder *pBookmarkFolder);
	void			InsertBookmarkFolderIntoListView(HWND hListView,CBookmarkFolder *pBookmarkFolder,int iPosition);
	void			InsertBookmarkIntoListView(HWND hListView,CBookmark *pBookmark,int iPosition);
	void			InsertBookmarkItemIntoListView(HWND hListView,const std::wstring &strName,UINT uID,int iPosition);
}

#endif