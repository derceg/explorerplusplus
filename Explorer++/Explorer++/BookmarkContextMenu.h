// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkItem.h"
#include "BookmarkTree.h"
#include "CoreInterface.h"

class BookmarkContextMenu
{
public:

	BookmarkContextMenu(BookmarkTree *bookmarkTree, HMODULE resourceModule, IExplorerplusplus *expp);

	BOOL ShowMenu(HWND parentWindow, BookmarkItem *bookmarkItem, const POINT &pt, bool recursive = false);
	bool IsShowingMenu() const;

private:

	void OnMenuItemSelected(int menuItemId, BookmarkItem *bookmarkItem, HWND parentWindow);
	void OnNewBookmarkItem(BookmarkItem::Type type, HWND parentWindow);
	void OnEditBookmarkItem(BookmarkItem *bookmarkItem, HWND parentWindow);

	BookmarkTree *m_bookmarkTree;
	HMODULE m_resourceModule;
	IExplorerplusplus *m_expp;
	bool m_showingMenu;
};