// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkContextMenu.h"

class BookmarkItem;
class BookmarkTree;
class CoreInterface;
class Navigator;

class BookmarkMenuController
{
public:
	BookmarkMenuController(BookmarkTree *bookmarkTree, CoreInterface *coreInterface,
		Navigator *navigator, HWND parentWindow);

	void OnMenuItemSelected(const BookmarkItem *bookmarkItem, bool isCtrlKeyDown,
		bool isShiftKeyDown);
	void OnMenuItemMiddleClicked(const BookmarkItem *bookmarkItem, bool isCtrlKeyDown,
		bool isShiftKeyDown);
	void OnMenuItemRightClicked(BookmarkItem *bookmarkItem, const POINT &pt);

private:
	CoreInterface *m_coreInterface;
	Navigator *m_navigator;
	HWND m_parentWindow;
	BookmarkContextMenu m_bookmarkContextMenu;
};
