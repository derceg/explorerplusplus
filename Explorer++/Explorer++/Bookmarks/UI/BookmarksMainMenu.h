// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/UI/BookmarkMenuBuilder.h"
#include "MenuHelper.h"
#include <boost/signals2.hpp>
#include <wil/resource.h>

class BookmarkTree;
class CoreInterface;
class IconFetcher;

class BookmarksMainMenu
{
public:
	BookmarksMainMenu(CoreInterface *coreInterface, IconFetcher *iconFetcher,
		BookmarkTree *bookmarkTree, const MenuIdRange &menuIdRange);
	~BookmarksMainMenu();

	void OnMenuItemClicked(int menuItemId);

private:
	void OnMainMenuPreShow(HMENU mainMenu);
	wil::unique_hmenu BuildMainBookmarksMenu(std::vector<wil::unique_hbitmap> &menuImages,
		BookmarkMenuBuilder::MenuInfo &menuInfo);
	void AddBookmarkItemsToMenu(HMENU menu, const MenuIdRange &menuIdRange, int position,
		std::vector<wil::unique_hbitmap> &menuImages, BookmarkMenuBuilder::MenuInfo &menuInfo);
	void AddOtherBookmarksToMenu(HMENU menu, const MenuIdRange &menuIdRange, int position,
		std::vector<wil::unique_hbitmap> &menuImages, BookmarkMenuBuilder::MenuInfo &menuInfo);

	CoreInterface *m_coreInterface;
	BookmarkTree *m_bookmarkTree;
	const MenuIdRange m_menuIdRange;
	BookmarkMenuBuilder m_menuBuilder;

	wil::unique_hmenu m_bookmarksMenu;
	std::vector<wil::unique_hbitmap> m_menuImages;
	BookmarkMenuBuilder::MenuInfo m_menuInfo;

	std::vector<boost::signals2::scoped_connection> m_connections;
};
