// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkItem.h"
#include "MenuHelper.h"
#include "../Helper/DpiCompatibility.h"
#include <boost/functional/hash.hpp>
#include <functional>
#include <unordered_map>
#include <utility>

class BookmarkIconManager;
class IconFetcher;
__interface IExplorerplusplus;

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

	using IncludePredicate = std::function<bool(const BookmarkItem *bookmarkItem)>;

	BookmarkMenuBuilder(IExplorerplusplus *expp, IconFetcher *iconFetcher, HMODULE resourceModule);

	BOOL BuildMenu(HWND parentWindow, HMENU menu, BookmarkItem *bookmarkItem,
		const MenuIdRange &menuIdRange, int startPosition, ItemIdMap &itemIdMap,
		std::vector<wil::unique_hbitmap> &menuImages, ItemPositionMap *itemPositionMap = nullptr,
		IncludePredicate includePredicate = nullptr);

private:
	BOOL BuildMenu(HMENU menu, BookmarkItem *bookmarkItem, int startPosition, ItemIdMap &itemIdMap,
		BookmarkIconManager &bookmarkIconManager, std::vector<wil::unique_hbitmap> &menuImages,
		ItemPositionMap *itemPositionMap, bool applyIncludePredicate,
		IncludePredicate includePredicate);
	BOOL AddEmptyBookmarkFolderToMenu(
		HMENU menu, BookmarkItem *bookmarkItem, int position, ItemPositionMap *itemPositionMap);
	BOOL AddBookmarkFolderToMenu(HMENU menu, BookmarkItem *bookmarkItem, int position,
		ItemIdMap &itemIdMap, BookmarkIconManager &bookmarkIconManager,
		std::vector<wil::unique_hbitmap> &menuImages, ItemPositionMap *itemPositionMap);
	BOOL AddBookmarkToMenu(HMENU menu, BookmarkItem *bookmarkItem, int position,
		ItemIdMap &itemIdMap, BookmarkIconManager &bookmarkIconManager,
		std::vector<wil::unique_hbitmap> &menuImages, ItemPositionMap *itemPositionMap);
	void AddIconToMenuItem(HMENU menu, int position, const BookmarkItem *bookmarkItem,
		BookmarkIconManager &bookmarkIconManager, std::vector<wil::unique_hbitmap> &menuImages);

	IExplorerplusplus *m_expp;
	IconFetcher *m_iconFetcher;
	HMODULE m_resourceModule;
	MenuIdRange m_menuIdRange;
	int m_idCounter;
	DpiCompatibility m_dpiCompat;
};