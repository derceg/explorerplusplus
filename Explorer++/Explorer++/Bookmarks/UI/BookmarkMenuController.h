// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkContextMenu.h"

class AcceleratorManager;
class BookmarkItem;
class BookmarkTree;
class BrowserWindow;
class CoreInterface;
class ResourceLoader;
class ThemeManager;

class BookmarkMenuController
{
public:
	BookmarkMenuController(BookmarkTree *bookmarkTree, BrowserWindow *browserWindow,
		CoreInterface *coreInterface, const AcceleratorManager *acceleratorManager,
		const ResourceLoader *resourceLoader, HWND parentWindow, ThemeManager *themeManager);

	void OnMenuItemSelected(const BookmarkItem *bookmarkItem, bool isCtrlKeyDown,
		bool isShiftKeyDown);
	void OnMenuItemMiddleClicked(const BookmarkItem *bookmarkItem, bool isCtrlKeyDown,
		bool isShiftKeyDown);
	void OnMenuItemRightClicked(BookmarkItem *bookmarkItem, const POINT &pt);

private:
	BrowserWindow *m_browserWindow = nullptr;
	CoreInterface *m_coreInterface = nullptr;
	HWND m_parentWindow;
	BookmarkContextMenu m_bookmarkContextMenu;
};
