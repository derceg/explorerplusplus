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
	typedef boost::variant<CBookmarkFolder &,CBookmark &> variantBookmark_t;

	enum SortMode_t
	{
		SM_NAME = 1
	};

	int CALLBACK		SortByName(const variantBookmark_t BookmarkItem1,const variantBookmark_t BookmarkItem2);
	int CALLBACK		SortByLocation(const variantBookmark_t BookmarkItem1,const variantBookmark_t BookmarkItem2);
	int CALLBACK		SortByVisitDate(const variantBookmark_t BookmarkItem1,const variantBookmark_t BookmarkItem2);
	int CALLBACK		SortByVisitCount(const variantBookmark_t BookmarkItem1,const variantBookmark_t BookmarkItem2);
	int CALLBACK		SortByAdded(const variantBookmark_t BookmarkItem1,const variantBookmark_t BookmarkItem2);
	int CALLBACK		SortByLastModified(const variantBookmark_t BookmarkItem1,const variantBookmark_t BookmarkItem2);

	variantBookmark_t	GetBookmarkItem(CBookmarkFolder &ParentBookmarkFolder,const GUID &guid);
}

class CBookmarkTreeView
{
public:

	CBookmarkTreeView(HWND hTreeView);
	~CBookmarkTreeView();

	LRESULT CALLBACK	TreeViewProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);

	void			InsertFoldersIntoTreeView(const CBookmarkFolder &BookmarkFolder,const GUID &guidSelected,const NBookmarkHelper::setExpansion_t &setExpansion);
	HTREEITEM		InsertFolderIntoTreeView(HTREEITEM hParent,const CBookmarkFolder &BookmarkFolder,const GUID &guidSelected,const NBookmarkHelper::setExpansion_t &setExpansion);
	CBookmarkFolder	&GetBookmarkFolderFromTreeView(HTREEITEM hItem,CBookmarkFolder &RootBookmarkFolder);

	void			SelectFolder(const GUID &guid);

private:

	typedef std::unordered_map<GUID,HTREEITEM,NBookmarkHelper::GuidHash,NBookmarkHelper::GuidEq> ItemMap_t;

	void	InsertFoldersIntoTreeViewRecursive(HTREEITEM hParent,const CBookmarkFolder &BookmarkFolder,const GUID &guidSelected,const NBookmarkHelper::setExpansion_t &setExpansion);
	void	OnTvnDeleteItem(NMTREEVIEW *pnmtv);

	HWND							m_hTreeView;
	HIMAGELIST						m_himl;

	std::unordered_map<UINT,GUID>	m_mapID;
	ItemMap_t						m_mapItem;
	UINT							m_uIDCounter;
};

class CBookmarkListView
{
public:

	CBookmarkListView(HWND hListView);
	~CBookmarkListView();

	void	InsertBookmarksIntoListView(const CBookmarkFolder &BookmarkFolder);
	void	InsertBookmarkFolderIntoListView(const CBookmarkFolder &BookmarkFolder,int iPosition);
	void	InsertBookmarkIntoListView(const CBookmark &Bookmark,int iPosition);
	NBookmarkHelper::variantBookmark_t	GetBookmarkItemFromListView(CBookmarkFolder &ParentBookmarkFolder,int iItem);

private:

	void	InsertBookmarkItemIntoListView(const std::wstring &strName,const GUID &guid,int iPosition);

	HWND							m_hListView;
	HIMAGELIST						m_himl;

	std::unordered_map<UINT,GUID>	m_mapID;
	UINT							m_uIDCounter;
};

#endif