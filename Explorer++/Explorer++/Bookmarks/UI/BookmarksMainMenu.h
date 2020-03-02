// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/UI/BookmarkMenuBuilder.h"
#include "MenuHelper.h"
#include "../Helper/DpiCompatibility.h"
#include <boost/signals2.hpp>
#include <wil/resource.h>

class BookmarkTree;
__interface IExplorerplusplus;

class BookmarksMainMenu
{
public:
	BookmarksMainMenu(
		IExplorerplusplus *expp, BookmarkTree *bookmarkTree, const MenuIdRange &menuIdRange);
	~BookmarksMainMenu();

	void OnMenuItemClicked(int menuItemId);

private:
	void OnMainMenuPreShow(HMENU mainMenu);
	wil::unique_hmenu BuildMainBookmarksMenu(std::vector<wil::unique_hbitmap> &menuImages,
		BookmarkMenuBuilder::ItemIdMap &menuItemIdMappings);
	void AddBookmarkItemsToMenu(
		HMENU menu, int position, BookmarkMenuBuilder::ItemIdMap &menuItemIdMappings);
	void AddOtherBookmarksToMenu(
		HMENU menu, int position, BookmarkMenuBuilder::ItemIdMap &menuItemIdMappings);

	IExplorerplusplus *m_expp;
	BookmarkTree *m_bookmarkTree;
	MenuIdRange m_menuIdRange;
	BookmarkMenuBuilder m_menuBuilder;

	wil::unique_hmenu m_bookmarksMenu;

	DpiCompatibility m_dpiCompat;
	std::vector<wil::unique_hbitmap> m_menuImages;

	BookmarkMenuBuilder::ItemIdMap m_menuItemIdMappings;

	std::vector<boost::signals2::scoped_connection> m_connections;
};