// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkContextMenu.h"

class BookmarkItem;
class BookmarkTree;
class BrowserWindow;
class CoreInterface;
class IconResourceLoader;

class BookmarkMenuController
{
public:
	BookmarkMenuController(BookmarkTree *bookmarkTree, BrowserWindow *browserWindow,
		CoreInterface *coreInterface, const IconResourceLoader *iconResourceLoader,
		HWND parentWindow);

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
