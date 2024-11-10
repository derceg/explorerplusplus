// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

namespace BookmarkStorage
{
static inline const wchar_t BOOKMARKS_TOOLBAR_NODE_NAME[] = L"BookmarksToolbar";
static inline const wchar_t BOOKMARKS_MENU_NODE_NAME[] = L"BookmarksMenu";
static inline const wchar_t OTHER_BOOKMARKS_NODE_NAME[] = L"OtherBookmarks";

// Used to load v1 bookmarks from the registry/config file. This enum should not
// be changed.
enum class BookmarkTypeV1
{
	Folder = 0,
	Bookmark = 1
};
}
