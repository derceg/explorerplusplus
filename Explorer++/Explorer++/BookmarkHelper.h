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

	void			InsertBookmarksIntoListView(HWND hListView,CBookmarkFolder *pBookmarkFolder);
	void			InsertBookmarkFolderIntoListView(HWND hListView,CBookmarkFolder *pBookmarkFolder,int iPosition);
	void			InsertBookmarkIntoListView(HWND hListView,CBookmark *pBookmark,int iPosition);
	void			InsertBookmarkItemIntoListView(HWND hListView,const std::wstring &strName,UINT uID,int iPosition);
}

class CBookmarkTreeView
{
public:

	CBookmarkTreeView(HWND hTreeView) :
		m_hTreeView(hTreeView),
		m_uIDCounter(0)
	{
		
	}

	~CBookmarkTreeView()
	{

	}

	void			InsertFoldersIntoTreeView(HWND hTreeView,CBookmarkFolder *pBookmarkFolder,const GUID &guidSelected,const NBookmarkHelper::setExpansion_t &setExpansion);
	HTREEITEM		InsertFolderIntoTreeView(HWND hTreeView,HTREEITEM hParent,CBookmarkFolder *pBookmarkFolder,const GUID &guidSelected,const NBookmarkHelper::setExpansion_t &setExpansion);
	CBookmarkFolder	*GetBookmarkFolderFromTreeView(HWND hTreeView,HTREEITEM hItem,CBookmarkFolder *pRootBookmarkFolder);

private:

	void	InsertFoldersIntoTreeViewRecursive(HWND hTreeView,HTREEITEM hParent,CBookmarkFolder *pBookmarkFolder,const GUID &guidSelected,const NBookmarkHelper::setExpansion_t &setExpansion);

	HWND							m_hTreeView;

	std::unordered_map<UINT,GUID>	m_mapID;
	UINT							m_uIDCounter;
};

#endif