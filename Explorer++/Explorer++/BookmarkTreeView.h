#pragma once

#include "BookmarkHelper.h"
#include "../Helper/Bookmark.h"
#include <unordered_map>

class CBookmarkTreeView
{
	friend LRESULT CALLBACK BookmarkTreeViewProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

public:

	CBookmarkTreeView(HWND hTreeView, CBookmarkFolder *pAllBookmarks, const GUID &guidSelected, const NBookmarkHelper::setExpansion_t &setExpansion);
	~CBookmarkTreeView();

	CBookmarkFolder					&GetBookmarkFolderFromTreeView(HTREEITEM hItem);

	HTREEITEM						BookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder, const CBookmarkFolder &BookmarkFolder, std::size_t Position);
	void							BookmarkFolderModified(const GUID &guid);
	void							BookmarkFolderRemoved(const GUID &guid);

	void							SelectFolder(const GUID &guid);

private:

	typedef std::unordered_map<GUID, HTREEITEM, NBookmarkHelper::GuidHash, NBookmarkHelper::GuidEq> ItemMap_t;

	LRESULT CALLBACK				TreeViewProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	void							SetupTreeView(const GUID &guidSelected, const NBookmarkHelper::setExpansion_t &setExpansion);

	HTREEITEM						InsertFolderIntoTreeView(HTREEITEM hParent, const CBookmarkFolder &BookmarkFolder, std::size_t Position);
	void							InsertFoldersIntoTreeViewRecursive(HTREEITEM hParent, const CBookmarkFolder &BookmarkFolder);

	void							OnTvnDeleteItem(NMTREEVIEW *pnmtv);

	HWND							m_hTreeView;
	HIMAGELIST						m_himl;

	CBookmarkFolder					*m_pAllBookmarks;

	std::unordered_map<UINT, GUID>	m_mapID;
	ItemMap_t						m_mapItem;
	UINT							m_uIDCounter;
};