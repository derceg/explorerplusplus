// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/UI/BookmarkColumn.h"
#include <compare>

class BookmarkItem;

UINT GetBookmarkColumnStringId(BookmarkColumn column);
std::weak_ordering CompareBookmarksByColumn(BookmarkColumn column, const BookmarkItem *firstItem,
	const BookmarkItem *secondItem);
