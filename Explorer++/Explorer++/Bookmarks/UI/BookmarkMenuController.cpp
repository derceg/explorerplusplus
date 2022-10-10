// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkMenuController.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkItem.h"

BookmarkMenuController::BookmarkMenuController(CoreInterface *coreInterface, Navigator *navigator) :
	m_coreInterface(coreInterface),
	m_navigator(navigator)
{
}

void BookmarkMenuController::OnBookmarkMenuItemSelected(const BookmarkItem *bookmarkItem)
{
	assert(bookmarkItem->IsBookmark());

	BookmarkHelper::OpenBookmarkItemWithDisposition(bookmarkItem, OpenFolderDisposition::CurrentTab,
		m_coreInterface, m_navigator);
}
