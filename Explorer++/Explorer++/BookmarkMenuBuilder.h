// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkItem.h"
#include "MenuHelper.h"
#include <unordered_map>

class BookmarkMenuBuilder
{
public:

	using ItemMap = std::unordered_map<int, const BookmarkItem *>;

	BookmarkMenuBuilder(HMODULE resourceModule);

	BOOL BuildMenu(HMENU menu, const BookmarkItem *bookmarkItem, const MenuIdRange &menuIdRange,
		int startPosition, ItemMap &itemMap);

private:

	BOOL BuildMenu(HMENU menu, const BookmarkItem *bookmarkItem, int startPosition, ItemMap &itemMap);
	BOOL AddEmptyBookmarkFolderToMenu(HMENU menu, int position);
	BOOL AddBookmarkFolderToMenu(HMENU menu, const BookmarkItem *bookmarkItem, int position, ItemMap &itemMap);
	BOOL AddBookmarkToMenu(HMENU menu, const BookmarkItem *bookmarkItem, int position, ItemMap &itemMap);

	HMODULE m_resourceModule;
	MenuIdRange m_menuIdRange;
	int m_idCounter;
};