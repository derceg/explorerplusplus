// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkItem.h"
#include "Bookmarks/UI/BookmarkContextMenuController.h"

class BookmarkTree;
__interface IExplorerplusplus;

class BookmarkContextMenu
{
public:
	BookmarkContextMenu(
		BookmarkTree *bookmarkTree, HMODULE resourceModule, IExplorerplusplus *expp);

	BOOL ShowMenu(HWND parentWindow, BookmarkItem *parentFolder,
		const RawBookmarkItems &bookmarkItems, const POINT &ptScreen, bool recursive = false);
	bool IsShowingMenu() const;

private:
	void SetUpMenu(HMENU menu, const RawBookmarkItems &bookmarkItems);
	void SetMenuItemStates(HMENU menu);

	HMODULE m_resourceModule;
	BookmarkContextMenuController m_controller;
	bool m_showingMenu;
};