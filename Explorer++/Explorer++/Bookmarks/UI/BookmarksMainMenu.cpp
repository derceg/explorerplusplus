// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarksMainMenu.h"
#include "Bookmarks/BookmarkIconManager.h"
#include "Bookmarks/BookmarkTree.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "TabContainer.h"
#include "../Helper/DpiCompatibility.h"

BookmarksMainMenu::BookmarksMainMenu(IExplorerplusplus *expp, IconFetcher *iconFetcher,
	BookmarkTree *bookmarkTree, const MenuIdRange &menuIdRange) :
	m_expp(expp),
	m_bookmarkTree(bookmarkTree),
	m_menuIdRange(menuIdRange),
	m_menuBuilder(expp, iconFetcher, expp->GetLanguageModule())
{
	m_connections.push_back(expp->AddMainMenuPreShowObserver(
		std::bind(&BookmarksMainMenu::OnMainMenuPreShow, this, std::placeholders::_1)));
}

BookmarksMainMenu::~BookmarksMainMenu()
{
	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_SUBMENU;
	mii.hSubMenu = nullptr;
	SetMenuItemInfo(GetMenu(m_expp->GetMainWindow()), IDM_BOOKMARKS, FALSE, &mii);
}

void BookmarksMainMenu::OnMainMenuPreShow(HMENU mainMenu)
{
	std::vector<wil::unique_hbitmap> menuImages;
	BookmarkMenuBuilder::ItemIdMap menuItemIdMappings;
	auto bookmarksMenu = BuildMainBookmarksMenu(menuImages, menuItemIdMappings);

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_SUBMENU;
	mii.hSubMenu = bookmarksMenu.get();
	SetMenuItemInfo(mainMenu, IDM_BOOKMARKS, FALSE, &mii);

	m_bookmarksMenu = std::move(bookmarksMenu);
	m_menuImages = std::move(menuImages);
	m_menuItemIdMappings = menuItemIdMappings;
}

wil::unique_hmenu BookmarksMainMenu::BuildMainBookmarksMenu(
	std::vector<wil::unique_hbitmap> &menuImages,
	BookmarkMenuBuilder::ItemIdMap &menuItemIdMappings)
{
	wil::unique_hmenu menu(CreatePopupMenu());

	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(m_expp->GetMainWindow());

	std::wstring bookmarkThisTabText =
		ResourceHelper::LoadString(m_expp->GetLanguageModule(), IDS_MENU_BOOKMARK_THIS_TAB);

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_ID | MIIM_STRING;
	mii.wID = IDM_BOOKMARKS_BOOKMARKTHISTAB;
	mii.dwTypeData = bookmarkThisTabText.data();
	InsertMenuItem(menu.get(), 0, TRUE, &mii);

	ResourceHelper::SetMenuItemImage(menu.get(), IDM_BOOKMARKS_BOOKMARKTHISTAB,
		m_expp->GetIconResourceLoader(), Icon::AddBookmark, dpi, menuImages);

	std::wstring bookmarkAllTabsText =
		ResourceHelper::LoadString(m_expp->GetLanguageModule(), IDS_MENU_BOOKMARK_ALL_TABS);

	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_ID | MIIM_STRING;
	mii.wID = IDM_BOOKMARKS_BOOKMARK_ALL_TABS;
	mii.dwTypeData = bookmarkAllTabsText.data();
	InsertMenuItem(menu.get(), 1, TRUE, &mii);

	std::wstring manageBookmarksText =
		ResourceHelper::LoadString(m_expp->GetLanguageModule(), IDS_MENU_MANAGE_BOOKMARKS);

	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_ID | MIIM_STRING;
	mii.wID = IDM_BOOKMARKS_MANAGEBOOKMARKS;
	mii.dwTypeData = manageBookmarksText.data();
	InsertMenuItem(menu.get(), 2, TRUE, &mii);

	ResourceHelper::SetMenuItemImage(menu.get(), IDM_BOOKMARKS_MANAGEBOOKMARKS,
		m_expp->GetIconResourceLoader(), Icon::Bookmarks, dpi, menuImages);

	int nextMenuItemId;
	AddBookmarkItemsToMenu(menu.get(), m_menuIdRange, GetMenuItemCount(menu.get()), menuImages,
		menuItemIdMappings, &nextMenuItemId);

	AddOtherBookmarksToMenu(menu.get(), { nextMenuItemId, m_menuIdRange.endId },
		GetMenuItemCount(menu.get()), menuImages, menuItemIdMappings);

	return menu;
}

void BookmarksMainMenu::AddBookmarkItemsToMenu(HMENU menu, const MenuIdRange &menuIdRange,
	int position, std::vector<wil::unique_hbitmap> &menuImages,
	BookmarkMenuBuilder::ItemIdMap &menuItemIdMappings, int *maxMenuItemId)
{
	BookmarkItem *bookmarksMenuFolder = m_bookmarkTree->GetBookmarksMenuFolder();

	if (bookmarksMenuFolder->GetChildren().empty())
	{
		return;
	}

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_FTYPE;
	mii.fType = MFT_SEPARATOR;
	InsertMenuItem(menu, position++, TRUE, &mii);

	m_menuBuilder.BuildMenu(m_expp->GetMainWindow(), menu, bookmarksMenuFolder, menuIdRange,
		position, menuItemIdMappings, menuImages, nullptr, maxMenuItemId);
}

void BookmarksMainMenu::AddOtherBookmarksToMenu(HMENU menu, const MenuIdRange &menuIdRange,
	int position, std::vector<wil::unique_hbitmap> &menuImages,
	BookmarkMenuBuilder::ItemIdMap &menuItemIdMappings)
{
	BookmarkItem *otherBookmarksFolder = m_bookmarkTree->GetOtherBookmarksFolder();

	if (otherBookmarksFolder->GetChildren().empty())
	{
		return;
	}

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_FTYPE;
	mii.fType = MFT_SEPARATOR;
	InsertMenuItem(menu, position++, TRUE, &mii);

	// Note that as DestroyMenu is recursive, this menu will be destroyed when
	// its parent menu is.
	HMENU subMenu = CreatePopupMenu();
	m_menuBuilder.BuildMenu(m_expp->GetMainWindow(), subMenu, otherBookmarksFolder, menuIdRange, 0,
		menuItemIdMappings, menuImages);

	std::wstring otherBookmarksName = otherBookmarksFolder->GetName();

	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_SUBMENU | MIIM_STRING;
	mii.hSubMenu = subMenu;
	mii.dwTypeData = otherBookmarksName.data();
	InsertMenuItem(menu, position++, TRUE, &mii);
}

void BookmarksMainMenu::OnMenuItemClicked(int menuItemId)
{
	auto itr = m_menuItemIdMappings.find(menuItemId);

	if (itr == m_menuItemIdMappings.end())
	{
		return;
	}

	const BookmarkItem *bookmark = itr->second;

	assert(bookmark->IsBookmark());

	Tab &selectedTab = m_expp->GetTabContainer()->GetSelectedTab();
	selectedTab.GetShellBrowser()->GetNavigationController()->BrowseFolder(bookmark->GetLocation());
}