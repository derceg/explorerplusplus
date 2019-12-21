// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkMenuBuilder.h"
#include "BookmarkTree.h"
#include "CoreInterface.h"
#include "MenuHelper.h"
#include "../Helper/DpiCompatibility.h"
#include <boost/signals2.hpp>
#include <wil/resource.h>

class BookmarksMainMenu
{
public:

	BookmarksMainMenu(IExplorerplusplus *expp, BookmarkTree *bookmarkTree,
		const MenuIdRange &menuIdRange);
	~BookmarksMainMenu();

	void OnMenuItemClicked(int menuItemId);

private:

	void OnMainMenuPreShow(HMENU mainMenu);
	wil::unique_hmenu BuildMainBookmarksMenu(std::vector<wil::unique_hbitmap> &menuImages,
		BookmarkMenuBuilder::ItemMap &menuItemMappings);
	void AddBookmarkItemsToMenu(HMENU menu, int position, BookmarkMenuBuilder::ItemMap &menuItemMappings);
	void AddOtherBookmarksToMenu(HMENU menu, int position, BookmarkMenuBuilder::ItemMap &menuItemMappings);

	IExplorerplusplus *m_expp;
	BookmarkTree *m_bookmarkTree;
	MenuIdRange m_menuIdRange;
	BookmarkMenuBuilder m_menuBuilder;

	wil::unique_hmenu m_bookmarksMenu;

	DpiCompatibility m_dpiCompat;
	std::vector<wil::unique_hbitmap> m_menuImages;

	BookmarkMenuBuilder::ItemMap m_menuItemMappings;

	std::vector<boost::signals2::scoped_connection> m_connections;
};