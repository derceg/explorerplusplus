// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/UI/BookmarkMenuBuilder.h"
#include "Bookmarks/UI/BookmarkMenuController.h"
#include "../Helper/WinRTBaseWrapper.h"
#include "../Helper/WindowSubclass.h"

class AcceleratorManager;
class BookmarkItem;
class BookmarkTree;
class BrowserWindow;
class CoreInterface;
class IconFetcher;
class IconResourceLoader;
class ResourceLoader;
class ThemeManager;

// Although it's not necessary, this class is effectively designed to be held
// for the lifetime of its parent class. Doing so is more efficient, as the
// parent window will only be subclassed once (on construction). It's then safe
// to call ShowMenu() as many times as needed.
class BookmarkMenu
{
public:
	BookmarkMenu(BookmarkTree *bookmarkTree, const ResourceLoader *resourceLoader,
		HINSTANCE resourceInstance, BrowserWindow *browserWindow, CoreInterface *coreInterface,
		const AcceleratorManager *acceleratorManager, const IconResourceLoader *iconResourceLoader,
		IconFetcher *iconFetcher, HWND parentWindow, ThemeManager *themeManager);

	BOOL ShowMenu(BookmarkItem *bookmarkItem, const POINT &pt,
		BookmarkMenuBuilder::IncludePredicate includePredicate = nullptr);

private:
	static const UINT MIN_ID = 1;
	static const UINT MAX_ID = 1000;

	LRESULT ParentWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void OnMenuRightButtonUp(HMENU menu, int index, const POINT &pt);
	void OnMenuMiddleButtonUp(const POINT &pt, bool isCtrlKeyDown, bool isShiftKeyDown);

	LRESULT OnMenuDrag(HMENU menu, int itemPosition);
	LRESULT OnMenuGetObject(MENUGETOBJECTINFO *objectInfo);

	void OnMenuItemSelected(UINT menuItemId, BookmarkMenuBuilder::ItemIdMap &menuItemIdMappings);

	BookmarkTree *m_bookmarkTree = nullptr;
	HWND m_parentWindow;
	BookmarkMenuBuilder m_menuBuilder;
	BookmarkMenuController m_controller;

	HMENU m_activeMenu = nullptr;
	BookmarkMenuBuilder::MenuInfo *m_menuInfo = nullptr;
	winrt::com_ptr<IDropTarget> m_dropTarget;

	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
};
