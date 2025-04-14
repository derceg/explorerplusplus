// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkItem.h"
#include <boost/functional/hash.hpp>
#include <functional>
#include <unordered_map>
#include <utility>

class BookmarkIconManager;
class IconFetcher;
class ResourceLoader;

class BookmarkMenuBuilder
{
public:
	enum class MenuItemType
	{
		// This item represents a bookmark/bookmark folder.
		BookmarkItem,

		// This is used when the parent folder contains no items. The associated BookmarkItem will
		// refer to the parent folder.
		EmptyItem
	};

	struct MenuIdRange
	{
		UINT startId;
		UINT endId;
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

	// Maps menu item IDs to bookmark items. Note that submenu items will have IDs set as well.
	using ItemIdMap = std::unordered_map<UINT, MenuItemEntry>;

	using IncludePredicate = std::function<bool(const BookmarkItem *bookmarkItem)>;

	// Contains information about the menu that was built.
	struct MenuInfo
	{
		// Can be used to retrieve items, based on their ID.
		ItemIdMap itemIdMap;

		UINT nextMenuId;
	};

	BookmarkMenuBuilder(const ResourceLoader *resourceLoader, IconFetcher *iconFetcher);

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

	const ResourceLoader *const m_resourceLoader;
	IconFetcher *const m_iconFetcher;
	MenuIdRange m_menuIdRange;
	UINT m_idCounter;
};
