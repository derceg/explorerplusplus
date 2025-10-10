// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkMenuController.h"
#include "BookmarkContextMenu.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkItem.h"
#include "BrowserWindow.h"
#include "NavigationHelper.h"
#include "PopupMenuView.h"
#include <glog/logging.h>

BookmarkMenuController::BookmarkMenuController(BookmarkTree *bookmarkTree, BrowserWindow *browser,
	const AcceleratorManager *acceleratorManager, const ResourceLoader *resourceLoader,
	HWND parentWindow, PlatformContext *platformContext) :
	m_bookmarkTree(bookmarkTree),
	m_browser(browser),
	m_acceleratorManager(acceleratorManager),
	m_resourceLoader(resourceLoader),
	m_parentWindow(parentWindow),
	m_platformContext(platformContext)
{
}

void BookmarkMenuController::OnMenuItemSelected(const BookmarkItem *bookmarkItem,
	bool isCtrlKeyDown, bool isShiftKeyDown)
{
	DCHECK(bookmarkItem->IsBookmark());

	BookmarkHelper::OpenBookmarkItemWithDisposition(bookmarkItem,
		DetermineOpenDisposition(false, isCtrlKeyDown, isShiftKeyDown), m_browser);
}

void BookmarkMenuController::OnMenuItemMiddleClicked(const BookmarkItem *bookmarkItem,
	bool isCtrlKeyDown, bool isShiftKeyDown)
{
	BookmarkHelper::OpenBookmarkItemWithDisposition(bookmarkItem,
		DetermineOpenDisposition(true, isCtrlKeyDown, isShiftKeyDown), m_browser);
}

void BookmarkMenuController::OnMenuItemRightClicked(BookmarkItem *bookmarkItem, const POINT &pt)
{
	PopupMenuView popupMenu(m_browser);
	BookmarkContextMenu contextMenu(&popupMenu, m_acceleratorManager, m_bookmarkTree,
		{ bookmarkItem }, m_resourceLoader, m_browser, m_parentWindow, m_platformContext);
	popupMenu.Show(m_parentWindow, pt);
}
