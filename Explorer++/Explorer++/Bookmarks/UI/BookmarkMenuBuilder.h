// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkItem.h"
#include "MenuHelper.h"
#include <boost/functional/hash.hpp>
#include <unordered_map>
#include <utility>

class BookmarkMenuBuilder
{
public:
	// Maps menu item IDs to bookmark items. Note that IDs will only be set for
	// bookmarks (and not bookmark folders).
	using ItemIdMap = std::unordered_map<int, BookmarkItem *>;

	// Maps menu item positions to bookmark items. Works for both bookmarks and
	// bookmark folders.
	using MenuPositionPair = std::pair<HMENU, int>;
	using ItemPositionMap =
		std::unordered_map<MenuPositionPair, BookmarkItem *, boost::hash<MenuPositionPair>>;

	BookmarkMenuBuilder(HMODULE resourceModule);

	BOOL BuildMenu(HMENU menu, BookmarkItem *bookmarkItem, const MenuIdRange &menuIdRange,
		int startPosition, ItemIdMap &itemIdMap, ItemPositionMap *itemPositionMap = nullptr);

private:
	BOOL BuildMenu(HMENU menu, BookmarkItem *bookmarkItem, int startPosition, ItemIdMap &itemIdMap,
		ItemPositionMap *itemPositionMap);
	BOOL AddEmptyBookmarkFolderToMenu(
		HMENU menu, BookmarkItem *bookmarkItem, int position, ItemPositionMap *itemPositionMap);
	BOOL AddBookmarkFolderToMenu(HMENU menu, BookmarkItem *bookmarkItem, int position,
		ItemIdMap &itemIdMap, ItemPositionMap *itemPositionMap);
	BOOL AddBookmarkToMenu(HMENU menu, BookmarkItem *bookmarkItem, int position,
		ItemIdMap &itemIdMap, ItemPositionMap *itemPositionMap);

	HMODULE m_resourceModule;
	MenuIdRange m_menuIdRange;
	int m_idCounter;
};