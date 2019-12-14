// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkHelper.h"
#include "CoreInterface.h"
#include "ResourceHelper.h"
#include "../Helper/Bookmark.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <wil/resource.h>
#include <unordered_map>
#include <unordered_set>

class CBookmarkTreeView
{
public:

	CBookmarkTreeView(HWND hTreeView, HINSTANCE hInstance, IExplorerplusplus *expp,
		CBookmarkFolder *pAllBookmarks, const std::wstring &guidSelected,
		const std::unordered_set<std::wstring> &setExpansion);

	CBookmarkFolder					&GetBookmarkFolderFromTreeView(HTREEITEM hItem);

	HTREEITEM						BookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder, const CBookmarkFolder &BookmarkFolder, std::size_t Position);
	void							BookmarkFolderModified(const std::wstring &guid);
	void							BookmarkFolderRemoved(const std::wstring &guid);

	void							CreateNewFolder();
	void							SelectFolder(const std::wstring &guid);

private:

	using ItemMap_t = std::unordered_map<std::wstring, HTREEITEM>;

	static const UINT_PTR			SUBCLASS_ID = 0;
	static const UINT_PTR			PARENT_SUBCLASS_ID = 0;

	static LRESULT CALLBACK			BookmarkTreeViewProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK				TreeViewProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK			BookmarkTreeViewParentProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK				TreeViewParentProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK			TreeViewEditProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK				TreeViewEditProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	void							SetupTreeView(const std::wstring &guidSelected, const std::unordered_set<std::wstring> &setExpansion);

	HTREEITEM						InsertFolderIntoTreeView(HTREEITEM hParent, const CBookmarkFolder &BookmarkFolder, std::size_t Position);
	void							InsertFoldersIntoTreeViewRecursive(HTREEITEM hParent, const CBookmarkFolder &BookmarkFolder);

	void							OnTvnKeyDown(NMTVKEYDOWN *pnmtvkd);
	void							OnTreeViewRename();
	void							OnTvnBeginLabelEdit();
	BOOL							OnTvnEndLabelEdit(NMTVDISPINFO *pnmtvdi);
	void							OnTvnDeleteItem(NMTREEVIEW *pnmtv);

	void							OnRClick(NMHDR *pnmhdr);

	HWND m_hTreeView;
	DpiCompatibility m_dpiCompat;
	wil::unique_himagelist m_imageList;
	IconImageListMapping m_imageListMappings;

	HINSTANCE m_instance;

	CBookmarkFolder *m_pAllBookmarks;

	std::unordered_map<UINT, std::wstring> m_mapID;
	ItemMap_t m_mapItem;
	UINT m_uIDCounter;

	bool m_bNewFolderCreated;
	std::wstring m_NewFolderGUID;

	std::vector<WindowSubclassWrapper> m_windowSubclasses;
};