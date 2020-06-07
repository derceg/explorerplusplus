// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkItem.h"
#include <gmock/gmock.h>

class BookmarkTreeObserver
{
public:
	MOCK_METHOD(void, OnBookmarkItemAdded, (BookmarkItem & bookmarkItem, size_t index));
	MOCK_METHOD(void, OnBookmarkItemUpdated,
		(BookmarkItem & bookmarkItem, BookmarkItem::PropertyType propertyType));
	MOCK_METHOD(void, OnBookmarkItemMoved,
		(BookmarkItem * bookmarkItem, const BookmarkItem *oldParent, size_t oldIndex,
			const BookmarkItem *newParent, size_t newIndex));
	MOCK_METHOD(void, OnBookmarkItemPreRemoval, (BookmarkItem & bookmarkItem));
	MOCK_METHOD(void, OnBookmarkItemRemoved, (const std::wstring &guid));
};