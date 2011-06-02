#ifndef BOOKMARKHELPER_INCLUDED
#define BOOKMARKHELPER_INCLUDED

#include <unordered_set>
#include <unordered_map>
#include "../Helper/Bookmark.h"

namespace NBookmarkHelper
{
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

	typedef std::unordered_set<GUID,GuidHash,GuidEq> setExpansion_t;
}

class CBookmarkTreeView
{
public:

	CBookmarkTreeView(HWND hTreeView);
	~CBookmarkTreeView();

	void			InsertFoldersIntoTreeView(CBookmarkFolder *pBookmarkFolder,const GUID &guidSelected,const NBookmarkHelper::setExpansion_t &setExpansion);
	HTREEITEM		InsertFolderIntoTreeView(HTREEITEM hParent,CBookmarkFolder *pBookmarkFolder,const GUID &guidSelected,const NBookmarkHelper::setExpansion_t &setExpansion);
	CBookmarkFolder	*GetBookmarkFolderFromTreeView(HTREEITEM hItem,CBookmarkFolder *pRootBookmarkFolder);

private:

	void	InsertFoldersIntoTreeViewRecursive(HTREEITEM hParent,CBookmarkFolder *pBookmarkFolder,const GUID &guidSelected,const NBookmarkHelper::setExpansion_t &setExpansion);

	HWND							m_hTreeView;
	HIMAGELIST						m_himl;

	std::unordered_map<UINT,GUID>	m_mapID;
	UINT							m_uIDCounter;
};

class CBookmarkListView
{
public:

	CBookmarkListView(HWND hListView);
	~CBookmarkListView();

	void	InsertBookmarksIntoListView(CBookmarkFolder *pBookmarkFolder);
	void	InsertBookmarkFolderIntoListView(CBookmarkFolder *pBookmarkFolder,int iPosition);
	void	InsertBookmarkIntoListView(CBookmark *pBookmark,int iPosition);
	std::pair<void *,NBookmarks::BookmarkType_t>	GetBookmarkItemFromListView(int iItem);

private:

	void	InsertBookmarkItemIntoListView(const std::wstring &strName,const GUID &guid,int iPosition);

	HWND							m_hListView;
	HIMAGELIST						m_himl;

	CBookmarkFolder					*m_pParentBookmarkFolder;

	std::unordered_map<UINT,GUID>	m_mapID;
	UINT							m_uIDCounter;
};

#endif