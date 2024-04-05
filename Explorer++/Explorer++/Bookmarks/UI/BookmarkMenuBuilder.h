// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkItem.h"
#include "MenuHelper.h"
#include <boost/functional/hash.hpp>
#include <functional>
#include <unordered_map>
#include <utility>

class BookmarkIconManager;
class CoreInterface;
class IconFetcher;

class BookmarkMenuBuilder
{
public:
	enum class MenuItemType
	{
		BookmarkItem,
		EmptyItem
	};

	struct MenuItemEntry
	{
		MenuItemEntry(BookmarkItem *bookmarkItem, MenuItemType menuItemType) :
			bookmarkItem(bookmarkItem),
			menuItemType(menuItemType)
		{
		}

		BookmarkItem *bookmarkItem;
		MenuItemType menuItemType;
	};

	// Maps menu item IDs to bookmark items. Note that IDs will only be set for bookmarks (and not
	// bookmark folders).
	using ItemIdMap = std::unordered_map<UINT, BookmarkItem *>;

	// Maps menu item positions to bookmark items. Works for both bookmarks and bookmark folders.
	using MenuPositionPair = std::pair<HMENU, int>;
	using ItemPositionMap =
		std::unordered_map<MenuPositionPair, MenuItemEntry, boost::hash<MenuPositionPair>>;

	using IncludePredicate = std::function<bool(const BookmarkItem *bookmarkItem)>;

	// Contains information about the menu that was built.
	struct MenuInfo
	{
		// Contains a set of all submenus and can be used to determine whether an arbitrary HMENU is
		// part of the returned menu.
		std::unordered_set<HMENU> menus;

		// Can be used to retrieve items, based on their ID/position.
		ItemIdMap itemIdMap;
		ItemPositionMap itemPositionMap;

		UINT nextMenuId;
	};

	BookmarkMenuBuilder(CoreInterface *coreInterface, IconFetcher *iconFetcher,
		HINSTANCE resourceInstance);

	BOOL BuildMenu(HWND parentWindow, HMENU menu, BookmarkItem *bookmarkItem,
		const MenuIdRange &menuIdRange, int startPosition,
		std::vector<wil::unique_hbitmap> &menuImages, MenuInfo &menuInfo,
		IncludePredicate includePredicate = nullptr);

private:
	BOOL BuildMenu(HMENU menu, BookmarkItem *bookmarkItem, int startPosition,
		BookmarkIconManager &bookmarkIconManager, std::vector<wil::unique_hbitmap> &menuImages,
		MenuInfo &menuInfo, bool applyIncludePredicate, IncludePredicate includePredicate);
	BOOL AddEmptyBookmarkFolderToMenu(HMENU menu, BookmarkItem *bookmarkItem, int position,
		MenuInfo &menuInfo);
	BOOL AddBookmarkFolderToMenu(HMENU menu, BookmarkItem *bookmarkItem, int position,
		BookmarkIconManager &bookmarkIconManager, std::vector<wil::unique_hbitmap> &menuImages,
		MenuInfo &menuInfo);
	BOOL AddBookmarkToMenu(HMENU menu, BookmarkItem *bookmarkItem, int position,
		BookmarkIconManager &bookmarkIconManager, std::vector<wil::unique_hbitmap> &menuImages,
		MenuInfo &menuInfo);
	void AddIconToMenuItem(HMENU menu, int position, const BookmarkItem *bookmarkItem,
		BookmarkIconManager &bookmarkIconManager, std::vector<wil::unique_hbitmap> &menuImages);

	CoreInterface *m_coreInterface;
	IconFetcher *m_iconFetcher;
	HINSTANCE m_resourceInstance;
	MenuIdRange m_menuIdRange;
	UINT m_idCounter;
};
