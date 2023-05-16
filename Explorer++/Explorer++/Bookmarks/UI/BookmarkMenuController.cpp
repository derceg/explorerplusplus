// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkMenuController.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkItem.h"
#include "CoreInterface.h"

BookmarkMenuController::BookmarkMenuController(BookmarkTree *bookmarkTree,
	CoreInterface *coreInterface, Navigator *navigator, HWND parentWindow) :
	m_coreInterface(coreInterface),
	m_navigator(navigator),
	m_parentWindow(parentWindow),
	m_bookmarkContextMenu(bookmarkTree, coreInterface->GetResourceInstance(), coreInterface,
		navigator)
{
}

void BookmarkMenuController::OnMenuItemSelected(const BookmarkItem *bookmarkItem,
	bool isCtrlKeyDown, bool isShiftKeyDown)
{
	assert(bookmarkItem->IsBookmark());

	BookmarkHelper::OpenBookmarkItemWithDisposition(bookmarkItem,
		m_navigator->DetermineOpenDisposition(false, isCtrlKeyDown, isShiftKeyDown),
		m_coreInterface, m_navigator);
}

void BookmarkMenuController::OnMenuItemMiddleClicked(const BookmarkItem *bookmarkItem,
	bool isCtrlKeyDown, bool isShiftKeyDown)
{
	BookmarkHelper::OpenBookmarkItemWithDisposition(bookmarkItem,
		m_navigator->DetermineOpenDisposition(true, isCtrlKeyDown, isShiftKeyDown), m_coreInterface,
		m_navigator);
}

void BookmarkMenuController::OnMenuItemRightClicked(BookmarkItem *bookmarkItem, const POINT &pt)
{
	m_bookmarkContextMenu.ShowMenu(m_parentWindow, bookmarkItem->GetParent(), { bookmarkItem }, pt,
		MenuType::Recursive);
}
