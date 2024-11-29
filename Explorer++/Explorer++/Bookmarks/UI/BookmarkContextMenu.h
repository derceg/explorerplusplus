// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkItem.h"
#include "Bookmarks/UI/BookmarkContextMenuController.h"

class BookmarkTree;
class BrowserWindow;
class CoreInterface;
class IconResourceLoader;
class ThemeManager;

enum class MenuType
{
	NonRecursive,
	Recursive
};

class BookmarkContextMenu
{
public:
	BookmarkContextMenu(BookmarkTree *bookmarkTree, HINSTANCE resourceInstance,
		BrowserWindow *browserWindow, CoreInterface *coreInterface,
		const IconResourceLoader *iconResourceLoader, ThemeManager *themeManager);

	BOOL ShowMenu(HWND parentWindow, BookmarkItem *parentFolder,
		const RawBookmarkItems &bookmarkItems, const POINT &ptScreen,
		MenuType menuType = MenuType::NonRecursive);
	bool IsShowingMenu() const;

private:
	void SetUpMenu(HMENU menu, const RawBookmarkItems &bookmarkItems);
	void SetMenuItemStates(HMENU menu, const RawBookmarkItems &bookmarkItems);

	BookmarkTree *const m_bookmarkTree;
	HINSTANCE m_resourceInstance;
	BookmarkContextMenuController m_controller;
	bool m_showingMenu;
};
