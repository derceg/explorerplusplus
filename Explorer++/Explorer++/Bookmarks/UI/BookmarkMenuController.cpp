// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkMenuController.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkItem.h"

BookmarkMenuController::BookmarkMenuController(CoreInterface *coreInterface) :
	m_coreInterface(coreInterface)
{
}

void BookmarkMenuController::OnBookmarkMenuItemSelected(const BookmarkItem *bookmarkItem)
{
	assert(bookmarkItem->IsBookmark());

	BookmarkHelper::OpenBookmarkItemWithDisposition(bookmarkItem, m_coreInterface,
		OpenFolderDisposition::CurrentTab);
}
