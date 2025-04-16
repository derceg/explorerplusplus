// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkMenuController.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkItem.h"
#include "BrowserWindow.h"
#include "CoreInterface.h"
#include "NavigationHelper.h"
#include <glog/logging.h>

BookmarkMenuController::BookmarkMenuController(BookmarkTree *bookmarkTree,
	BrowserWindow *browserWindow, CoreInterface *coreInterface,
	const AcceleratorManager *acceleratorManager, const ResourceLoader *resourceLoader,
	HWND parentWindow) :
	m_browserWindow(browserWindow),
	m_parentWindow(parentWindow),
	m_bookmarkContextMenu(bookmarkTree, resourceLoader, coreInterface->GetResourceInstance(),
		browserWindow, acceleratorManager)
{
}

void BookmarkMenuController::OnMenuItemSelected(const BookmarkItem *bookmarkItem,
	bool isCtrlKeyDown, bool isShiftKeyDown)
{
	DCHECK(bookmarkItem->IsBookmark());

	BookmarkHelper::OpenBookmarkItemWithDisposition(bookmarkItem,
		DetermineOpenDisposition(false, isCtrlKeyDown, isShiftKeyDown), m_browserWindow);
}

void BookmarkMenuController::OnMenuItemMiddleClicked(const BookmarkItem *bookmarkItem,
	bool isCtrlKeyDown, bool isShiftKeyDown)
{
	BookmarkHelper::OpenBookmarkItemWithDisposition(bookmarkItem,
		DetermineOpenDisposition(true, isCtrlKeyDown, isShiftKeyDown), m_browserWindow);
}

void BookmarkMenuController::OnMenuItemRightClicked(BookmarkItem *bookmarkItem, const POINT &pt)
{
	m_bookmarkContextMenu.ShowMenu(m_parentWindow, bookmarkItem->GetParent(), { bookmarkItem }, pt,
		MenuType::Recursive);
}
