// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BookmarkItem.h"
#include <functional>
#include <unordered_map>

class BookmarkMenu
{
public:

	using MenuCallback = std::function<void(const BookmarkItem *bookmarkItem)>;

	BookmarkMenu(HINSTANCE instance);

	BOOL ShowMenu(HWND parentWindow, const BookmarkItem *bookmarkItem, const POINT &pt,
		MenuCallback callback = nullptr);

private:

	BOOL BuildBookmarksMenu(HMENU menu, const BookmarkItem *bookmarkItem, int startPosition);
	BOOL AddEmptyBookmarkFolderToMenu(HMENU menu, int position);
	BOOL AddBookmarkFolderToMenu(HMENU menu, const BookmarkItem *bookmarkItem, int position);
	BOOL AddBookmarkToMenu(HMENU menu, const BookmarkItem *bookmarkItem, int position);
	void OnMenuItemSelected(int menuItemId, MenuCallback callback);

	HINSTANCE m_instance;

	int m_idCounter;
	std::unordered_map<int, const BookmarkItem *> m_menuItemMap;
};