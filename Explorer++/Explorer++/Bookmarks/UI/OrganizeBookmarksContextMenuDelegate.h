// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkHelper.h"

class BookmarkItem;

// This interface allows the organize bookmarks menu to delegate certain actions to a target view.
class OrganizeBookmarksContextMenuDelegate
{
public:
	virtual ~OrganizeBookmarksContextMenuDelegate() = default;

	virtual bool CanSelectAllItems() const = 0;
	virtual void SelectAllItems() = 0;

	virtual void CreateFolder(size_t index) = 0;

	virtual RawBookmarkItems GetSelectedItems() const = 0;
	virtual RawBookmarkItems GetSelectedChildItems(const BookmarkItem *targetFolder) const = 0;
	virtual void SelectItem(const BookmarkItem *bookmarkItem) = 0;
};
