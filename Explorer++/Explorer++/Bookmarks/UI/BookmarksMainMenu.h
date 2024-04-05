// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/UI/BookmarkMenuBuilder.h"
#include "Bookmarks/UI/BookmarkMenuController.h"
#include "MenuHelper.h"
#include <boost/signals2.hpp>
#include <wil/resource.h>

class BookmarkTree;
class BrowserWindow;
class CoreInterface;
class IconFetcher;

class BookmarksMainMenu
{
public:
	BookmarksMainMenu(BrowserWindow *browserWindow, CoreInterface *coreInterface,
		IconFetcher *iconFetcher, BookmarkTree *bookmarkTree, const MenuIdRange &menuIdRange);
	~BookmarksMainMenu();

	void OnMenuItemClicked(UINT menuItemId);

private:
	void OnMainMenuPreShow(HMENU mainMenu);
	wil::unique_hmenu BuildMainBookmarksMenu(std::vector<wil::unique_hbitmap> &menuImages,
		BookmarkMenuBuilder::MenuInfo &menuInfo);
	void AddBookmarkItemsToMenu(HMENU menu, const MenuIdRange &menuIdRange, int position,
		std::vector<wil::unique_hbitmap> &menuImages, BookmarkMenuBuilder::MenuInfo &menuInfo);
	void AddOtherBookmarksToMenu(HMENU menu, const MenuIdRange &menuIdRange, int position,
		std::vector<wil::unique_hbitmap> &menuImages, BookmarkMenuBuilder::MenuInfo &menuInfo);
	std::optional<std::wstring> MaybeGetMenuItemHelperText(HMENU menu, UINT id);
	bool OnMenuItemMiddleClicked(const POINT &pt, bool isCtrlKeyDown, bool isShiftKeyDown);
	bool OnMenuItemRightClicked(HMENU menu, int index, const POINT &pt);

	CoreInterface *m_coreInterface = nullptr;
	BookmarkTree *m_bookmarkTree = nullptr;
	const MenuIdRange m_menuIdRange;
	BookmarkMenuBuilder m_menuBuilder;

	wil::unique_hmenu m_bookmarksMenu;
	std::vector<wil::unique_hbitmap> m_menuImages;
	BookmarkMenuBuilder::MenuInfo m_menuInfo;

	BookmarkMenuController m_controller;

	std::vector<boost::signals2::scoped_connection> m_connections;
};
