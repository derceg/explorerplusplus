// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class BookmarkItem;

class BookmarkItemCreationDelegate
{
public:
	virtual ~BookmarkItemCreationDelegate() = default;

	virtual void CreateBookmark(BookmarkItem *parentFolder, size_t index) = 0;
	virtual void CreateFolder(BookmarkItem *parentFolder, size_t index) = 0;
};
