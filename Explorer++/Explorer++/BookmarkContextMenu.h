// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkHelper.h"
#include "BookmarkItem.h"
#include "BookmarkTree.h"
#include "CoreInterface.h"

class BookmarkContextMenu
{
public:

	BookmarkContextMenu(BookmarkTree *bookmarkTree, HMODULE resourceModule, IExplorerplusplus *expp);

	BOOL ShowMenu(HWND parentWindow, BookmarkItem *parentFolder, const RawBookmarkItems &bookmarkItems,
		const POINT &ptScreen, bool recursive = false);
	bool IsShowingMenu() const;

private:

	void SetUpMenu(HMENU menu, const RawBookmarkItems &bookmarkItems);
	void SetMenuItemStates(HMENU menu);

	void OnMenuItemSelected(int menuItemId, BookmarkItem *parentFolder, const RawBookmarkItems &bookmarkItems,
		HWND parentWindow);
	void OnOpenAll(const RawBookmarkItems &bookmarkItems);
	void OnNewBookmarkItem(BookmarkItem::Type type, BookmarkItem *parentFolder, HWND parentWindow);
	void OnCopy(const RawBookmarkItems &bookmarkItems, bool cut);
	void OnPaste(BookmarkItem *selectedBookmarkItem);
	void OnDelete(const RawBookmarkItems &bookmarkItems);
	void OnEditBookmarkItem(BookmarkItem *bookmarkItem, HWND parentWindow);

	BookmarkTree *m_bookmarkTree;
	HMODULE m_resourceModule;
	IExplorerplusplus *m_expp;
	bool m_showingMenu;
};