// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/Bookmark.h"
#include <functional>
#include <unordered_map>

class BookmarkMenu
{
public:

	BookmarkMenu(HINSTANCE instance);

	BOOL ShowMenu(HWND parentWindow, const CBookmarkFolder &parentBookmark, const POINT &pt,
		const std::function<void(const CBookmark &)> &callback = nullptr);

private:

	BOOL BuildBookmarksMenu(HMENU menu, const CBookmarkFolder &parent, int startPosition);
	BOOL AddEmptyBookmarkFolderToMenu(HMENU menu, int position);
	BOOL AddBookmarkFolderToMenu(HMENU menu, const CBookmarkFolder &bookmarkFolder, int position);
	BOOL AddBookmarkToMenu(HMENU menu, const CBookmark &bookmark, int position);
	void OnMenuItemSelected(int menuItemId, const std::function<void(const CBookmark &)> &callback = nullptr);

	HINSTANCE m_instance;

	int m_idCounter;
	std::unordered_map<int, const CBookmark *> m_menuItemMap;
};