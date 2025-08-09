// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkItem.h"

struct CopiedBookmark
{
	CopiedBookmark(const BookmarkItem &bookmark);

	const BookmarkItem::Type type;
	const std::wstring name;
	const std::wstring location;
};

bool operator==(const CopiedBookmark &copiedBookmark, const BookmarkItem &bookmarkItem);
