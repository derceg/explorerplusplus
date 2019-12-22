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
#include <wil/resource.h>

class CBookmarkListView
{
public:

	CBookmarkListView(HWND hListView, HMODULE resourceModule, BookmarkTree *bookmarkTree,
		IExplorerplusplus *expp);

	void NavigateToBookmarkFolder(BookmarkItem *bookmarkItem);
	BookmarkItem *GetBookmarkItemFromListView(int iItem);
	BookmarkItem *GetBookmarkItemFromListViewlParam(LPARAM lParam);

private:

	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	static LRESULT CALLBACK ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam,
		LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	int InsertBookmarkItemIntoListView(BookmarkItem *bookmarkItem, int position);

	void OnRClick(const NMITEMACTIVATE *itemActivate);
	BOOL OnBeginLabelEdit(const NMLVDISPINFO *dispInfo);
	BOOL OnEndLabelEdit(const NMLVDISPINFO *dispInfo);
	void OnKeyDown(const NMLVKEYDOWN *keyDown);
	void OnRename();

	HWND m_hListView;
	HMODULE m_resourceModule;
	DpiCompatibility m_dpiCompat;
	wil::unique_himagelist m_imageList;
	IconImageListMapping m_imageListMappings;

	BookmarkTree *m_bookmarkTree;
	BookmarkItem *m_currentBookmarkFolder;

	std::vector<WindowSubclassWrapper> m_windowSubclasses;
};