// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkItem.h"
#include "Bookmarks/UI/BookmarkContextMenuController.h"

class AcceleratorManager;
class BookmarkTree;
class BrowserWindow;
class ResourceLoader;

enum class MenuType
{
	NonRecursive,
	Recursive
};

class BookmarkContextMenu
{
public:
	BookmarkContextMenu(BookmarkTree *bookmarkTree, const ResourceLoader *resourceLoader,
		HINSTANCE resourceInstance, BrowserWindow *browserWindow,
		const AcceleratorManager *acceleratorManager);

	BOOL ShowMenu(HWND parentWindow, BookmarkItem *parentFolder,
		const RawBookmarkItems &bookmarkItems, const POINT &ptScreen,
		MenuType menuType = MenuType::NonRecursive);
	bool IsShowingMenu() const;

private:
	void SetUpMenu(HMENU menu, const RawBookmarkItems &bookmarkItems);
	void SetMenuItemStates(HMENU menu, const RawBookmarkItems &bookmarkItems);

	BookmarkTree *const m_bookmarkTree;
	const ResourceLoader *const m_resourceLoader;
	HINSTANCE m_resourceInstance;
	BookmarkContextMenuController m_controller;
	bool m_showingMenu;
};
