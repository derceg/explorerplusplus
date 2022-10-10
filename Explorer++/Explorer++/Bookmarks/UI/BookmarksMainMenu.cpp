// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarksMainMenu.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkTree.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "TabContainer.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/MenuHelper.h"

BookmarksMainMenu::BookmarksMainMenu(CoreInterface *coreInterface, Navigator *navigator,
	IconFetcher *iconFetcher, BookmarkTree *bookmarkTree, const MenuIdRange &menuIdRange) :
	m_coreInterface(coreInterface),
	m_navigator(navigator),
	m_bookmarkTree(bookmarkTree),
	m_menuIdRange(menuIdRange),
	m_menuBuilder(coreInterface, iconFetcher, coreInterface->GetResourceModule())
{
	m_connections.push_back(coreInterface->AddMainMenuPreShowObserver(
		std::bind_front(&BookmarksMainMenu::OnMainMenuPreShow, this)));
}

BookmarksMainMenu::~BookmarksMainMenu()
{
	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_SUBMENU;
	mii.hSubMenu = nullptr;
	SetMenuItemInfo(GetMenu(m_coreInterface->GetMainWindow()), IDM_BOOKMARKS, FALSE, &mii);
}

void BookmarksMainMenu::OnMainMenuPreShow(HMENU mainMenu)
{
	std::vector<wil::unique_hbitmap> menuImages;
	BookmarkMenuBuilder::MenuInfo menuInfo;
	auto bookmarksMenu = BuildMainBookmarksMenu(menuImages, menuInfo);

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_SUBMENU;
	mii.hSubMenu = bookmarksMenu.get();
	SetMenuItemInfo(mainMenu, IDM_BOOKMARKS, FALSE, &mii);

	m_bookmarksMenu = std::move(bookmarksMenu);
	m_menuImages = std::move(menuImages);
	m_menuInfo = menuInfo;
}

wil::unique_hmenu BookmarksMainMenu::BuildMainBookmarksMenu(
	std::vector<wil::unique_hbitmap> &menuImages, BookmarkMenuBuilder::MenuInfo &menuInfo)
{
	wil::unique_hmenu menu(CreatePopupMenu());

	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(m_coreInterface->GetMainWindow());

	std::wstring bookmarkThisTabText = ResourceHelper::LoadString(
		m_coreInterface->GetResourceModule(), IDS_MENU_BOOKMARK_THIS_TAB);
	MenuHelper::AddStringItem(menu.get(), IDM_BOOKMARKS_BOOKMARKTHISTAB, bookmarkThisTabText, 0,
		TRUE);
	ResourceHelper::SetMenuItemImage(menu.get(), IDM_BOOKMARKS_BOOKMARKTHISTAB,
		m_coreInterface->GetIconResourceLoader(), Icon::AddBookmark, dpi, menuImages);

	std::wstring bookmarkAllTabsText = ResourceHelper::LoadString(
		m_coreInterface->GetResourceModule(), IDS_MENU_BOOKMARK_ALL_TABS);
	MenuHelper::AddStringItem(menu.get(), IDM_BOOKMARKS_BOOKMARK_ALL_TABS, bookmarkAllTabsText, 1,
		TRUE);

	std::wstring manageBookmarksText =
		ResourceHelper::LoadString(m_coreInterface->GetResourceModule(), IDS_MENU_MANAGE_BOOKMARKS);
	MenuHelper::AddStringItem(menu.get(), IDM_BOOKMARKS_MANAGEBOOKMARKS, manageBookmarksText, 2,
		TRUE);
	ResourceHelper::SetMenuItemImage(menu.get(), IDM_BOOKMARKS_MANAGEBOOKMARKS,
		m_coreInterface->GetIconResourceLoader(), Icon::Bookmarks, dpi, menuImages);

	AddBookmarkItemsToMenu(menu.get(), m_menuIdRange, GetMenuItemCount(menu.get()), menuImages,
		menuInfo);
	AddOtherBookmarksToMenu(menu.get(), { menuInfo.nextMenuId, m_menuIdRange.endId },
		GetMenuItemCount(menu.get()), menuImages, menuInfo);

	return menu;
}

void BookmarksMainMenu::AddBookmarkItemsToMenu(HMENU menu, const MenuIdRange &menuIdRange,
	int position, std::vector<wil::unique_hbitmap> &menuImages,
	BookmarkMenuBuilder::MenuInfo &menuInfo)
{
	BookmarkItem *bookmarksMenuFolder = m_bookmarkTree->GetBookmarksMenuFolder();

	if (bookmarksMenuFolder->GetChildren().empty())
	{
		return;
	}

	MenuHelper::AddSeparator(menu, position++, TRUE);

	m_menuBuilder.BuildMenu(m_coreInterface->GetMainWindow(), menu, bookmarksMenuFolder,
		menuIdRange, position, menuImages, menuInfo);
}

void BookmarksMainMenu::AddOtherBookmarksToMenu(HMENU menu, const MenuIdRange &menuIdRange,
	int position, std::vector<wil::unique_hbitmap> &menuImages,
	BookmarkMenuBuilder::MenuInfo &menuInfo)
{
	BookmarkItem *otherBookmarksFolder = m_bookmarkTree->GetOtherBookmarksFolder();

	if (otherBookmarksFolder->GetChildren().empty())
	{
		return;
	}

	MenuHelper::AddSeparator(menu, position++, TRUE);

	// Note that as DestroyMenu is recursive, this menu will be destroyed when its parent menu is.
	wil::unique_hmenu subMenu(CreatePopupMenu());
	m_menuBuilder.BuildMenu(m_coreInterface->GetMainWindow(), subMenu.get(), otherBookmarksFolder,
		menuIdRange, 0, menuImages, menuInfo);

	std::wstring otherBookmarksName = otherBookmarksFolder->GetName();
	MenuHelper::AddSubMenuItem(menu, otherBookmarksName, std::move(subMenu), position++, TRUE);
}

void BookmarksMainMenu::OnMenuItemClicked(int menuItemId)
{
	auto itr = m_menuInfo.itemIdMap.find(menuItemId);

	if (itr == m_menuInfo.itemIdMap.end())
	{
		return;
	}

	const BookmarkItem *bookmark = itr->second;

	assert(bookmark->IsBookmark());

	BookmarkHelper::OpenBookmarkItemWithDisposition(bookmark, OpenFolderDisposition::CurrentTab,
		m_coreInterface, m_navigator);
}
