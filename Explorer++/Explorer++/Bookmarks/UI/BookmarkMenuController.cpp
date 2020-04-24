// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkMenuController.h"
#include "Bookmarks/BookmarkItem.h"
#include "Navigation.h"

BookmarkMenuController::BookmarkMenuController(Navigation *navigation) : m_navigation(navigation)
{
}

void BookmarkMenuController::OnBookmarkMenuItemSelected(const BookmarkItem *bookmarkItem)
{
	assert(bookmarkItem->IsBookmark());
	m_navigation->BrowseFolderInCurrentTab(bookmarkItem->GetLocation().c_str());
}