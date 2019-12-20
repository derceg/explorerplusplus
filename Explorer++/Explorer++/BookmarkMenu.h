// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkItem.h"
#include "BookmarkMenuBuilder.h"
#include <functional>

class BookmarkMenu
{
public:

	using MenuCallback = std::function<void(const BookmarkItem *bookmarkItem)>;

	BookmarkMenu(HINSTANCE instance);

	BOOL ShowMenu(HWND parentWindow, const BookmarkItem *bookmarkItem, const POINT &pt,
		MenuCallback callback = nullptr);

private:

	static const int MIN_ID = 1;
	static const int MAX_ID = 1000;

	void OnMenuItemSelected(int menuItemId, BookmarkMenuBuilder::ItemMap &menuItemMappings,
		MenuCallback callback);

	HINSTANCE m_instance;
	BookmarkMenuBuilder m_menuBuilder;
};