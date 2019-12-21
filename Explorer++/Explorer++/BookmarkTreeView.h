// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkHelper.h"
#include "BookmarkItem.h"
#include "BookmarkTree.h"
#include "CoreInterface.h"
#include "ResourceHelper.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <boost/signals2.hpp>
#include <wil/resource.h>
#include <unordered_map>
#include <unordered_set>

class CBookmarkTreeView
{
public:

	CBookmarkTreeView(HWND hTreeView, HINSTANCE hInstance, IExplorerplusplus *expp,
		BookmarkTree *bookmarkTree, const std::wstring &guidSelected,
		const std::unordered_set<std::wstring> &setExpansion);

	BookmarkItem *GetBookmarkFolderFromTreeView(HTREEITEM hItem);

	void CreateNewFolder();
	void SelectFolder(const std::wstring &guid);

private:

	using ItemMap_t = std::unordered_map<std::wstring, HTREEITEM>;

	static const UINT_PTR SUBCLASS_ID = 0;
	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	static LRESULT CALLBACK BookmarkTreeViewProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK TreeViewProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK BookmarkTreeViewParentProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK TreeViewParentProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK TreeViewEditProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK TreeViewEditProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	void SetupTreeView(const std::wstring &guidSelected, const std::unordered_set<std::wstring> &setExpansion);

	void InsertFoldersIntoTreeViewRecursive(HTREEITEM hParent, BookmarkItem *bookmarkItem);
	HTREEITEM InsertFolderIntoTreeView(HTREEITEM hParent, BookmarkItem *bookmarkFolder, int position);

	void OnTvnKeyDown(NMTVKEYDOWN *pnmtvkd);
	void OnTreeViewRename();
	void OnTvnBeginLabelEdit();
	BOOL OnTvnEndLabelEdit(NMTVDISPINFO *pnmtvdi);
	void OnDelete();

	void OnRClick(NMHDR *pnmhdr);

	void OnBookmarkItemAdded(BookmarkItem &bookmarkItem, size_t index);
	void OnBookmarkItemUpdated(BookmarkItem &bookmarkItem, BookmarkItem::PropertyType propertyType);
	void OnBookmarkItemRemoved(const std::wstring &guid);

	HWND m_hTreeView;
	DpiCompatibility m_dpiCompat;
	wil::unique_himagelist m_imageList;
	IconImageListMapping m_imageListMappings;

	HINSTANCE m_instance;

	BookmarkTree *m_bookmarkTree;

	ItemMap_t m_mapItem;

	bool m_bNewFolderCreated;
	std::wstring m_NewFolderGUID;

	std::vector<WindowSubclassWrapper> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;
};