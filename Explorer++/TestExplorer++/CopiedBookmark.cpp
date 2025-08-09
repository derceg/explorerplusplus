// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "CopiedBookmark.h"

CopiedBookmark::CopiedBookmark(const BookmarkItem &bookmark) :
	type(bookmark.GetType()),
	name(bookmark.GetName()),
	location(bookmark.GetLocation())
{
}

bool operator==(const CopiedBookmark &copiedBookmark, const BookmarkItem &bookmarkItem)
{
	return copiedBookmark.type == bookmarkItem.GetType()
		&& copiedBookmark.name == bookmarkItem.GetName()
		&& copiedBookmark.location == bookmarkItem.GetLocation();
}
