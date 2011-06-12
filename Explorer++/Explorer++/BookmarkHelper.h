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
		SM_NAME = 1,
		SM_LOCATION = 2,
		SM_VISIT_DATE = 3,
		SM_VISIT_COUNT = 4,
		SM_ADDED = 5,
		SM_LAST_MODIFIED = 6
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

	CBookmarkTreeView(HWND hTreeView,CBookmarkFolder *pAllBookmarks,const GUID &guidSelected,const NBookmarkHelper::setExpansion_t &setExpansion);
	~CBookmarkTreeView();

	LRESULT CALLBACK	TreeViewProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);

	CBookmarkFolder		&GetBookmarkFolderFromTreeView(HTREEITEM hItem);

	void				BookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmarkFolder &BookmarkFolder);
	void				BookmarkFolderModified(const GUID &guid);

	void				SelectFolder(const GUID &guid);

private:

	typedef std::unordered_map<GUID,HTREEITEM,NBookmarkHelper::GuidHash,NBookmarkHelper::GuidEq> ItemMap_t;

	void				SetupTreeView(const GUID &guidSelected,const NBookmarkHelper::setExpansion_t &setExpansion);

	HTREEITEM			InsertFolderIntoTreeView(HTREEITEM hParent,const CBookmarkFolder &BookmarkFolder);
	void				InsertFoldersIntoTreeViewRecursive(HTREEITEM hParent,const CBookmarkFolder &BookmarkFolder);

	void				OnTvnDeleteItem(NMTREEVIEW *pnmtv);

	HWND							m_hTreeView;
	HIMAGELIST						m_himl;

	CBookmarkFolder					*m_pAllBookmarks;

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
	int		InsertBookmarkFolderIntoListView(const CBookmarkFolder &BookmarkFolder);
	int		InsertBookmarkFolderIntoListView(const CBookmarkFolder &BookmarkFolder,int iPosition);
	int		InsertBookmarkIntoListView(const CBookmark &Bookmark);
	int		InsertBookmarkIntoListView(const CBookmark &Bookmark,int iPosition);
	NBookmarkHelper::variantBookmark_t	GetBookmarkItemFromListView(CBookmarkFolder &ParentBookmarkFolder,int iItem);
	NBookmarkHelper::variantBookmark_t	GetBookmarkItemFromListViewlParam(CBookmarkFolder &ParentBookmarkFolder,LPARAM lParam);

private:

	int		InsertBookmarkItemIntoListView(const std::wstring &strName,const GUID &guid,bool bFolder,int iPosition);

	HWND							m_hListView;
	HIMAGELIST						m_himl;

	std::unordered_map<UINT,GUID>	m_mapID;
	UINT							m_uIDCounter;
};

/* Receives low-level bookmark notifications, and rebroadcasts them
via IPC to other Explorer++ processes. */
class CIPBookmarkItemNotifier : public NBookmark::IBookmarkItemNotification
{
public:

	CIPBookmarkItemNotifier();
	~CIPBookmarkItemNotifier();

	void	OnBookmarkItemModified(const GUID &guid);
	void	OnBookmarkAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmark &Bookmark);
	void	OnBookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmarkFolder &BookmarkFolder);
	void	OnBookmarkRemoved(const GUID &guid);
	void	OnBookmarkFolderRemoved(const GUID &guid);

private:
};

#endif