// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarksMainMenu.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "TabContainer.h"

BookmarksMainMenu::BookmarksMainMenu(IExplorerplusplus *expp, const BookmarkTree *bookmarkTree,
	const MenuIdRange &menuIdRange) :
	m_expp(expp),
	m_bookmarkTree(bookmarkTree),
	m_menuIdRange(menuIdRange),
	m_menuBuilder(expp->GetLanguageModule())
{
	m_connections.push_back(expp->AddMainMenuPreShowObserver(std::bind(&BookmarksMainMenu::OnMainMenuPreShow,
		this, std::placeholders::_1)));
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
	BookmarkMenuBuilder::ItemMap menuItemMappings;
	auto bookmarksMenu = BuildMainBookmarksMenu(menuImages, menuItemMappings);

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_SUBMENU;
	mii.hSubMenu = bookmarksMenu.get();
	SetMenuItemInfo(mainMenu, IDM_BOOKMARKS, FALSE, &mii);

	m_bookmarksMenu = std::move(bookmarksMenu);
	m_menuImages = std::move(menuImages);
	m_menuItemMappings = menuItemMappings;
}

wil::unique_hmenu BookmarksMainMenu::BuildMainBookmarksMenu(std::vector<wil::unique_hbitmap> &menuImages,
	BookmarkMenuBuilder::ItemMap &menuItemMappings)
{
	wil::unique_hmenu menu(CreatePopupMenu());

	int position = 0;

	std::wstring bookmarkThisTabText = ResourceHelper::LoadString(m_expp->GetLanguageModule(), IDS_MENU_BOOKMARK_THIS_TAB);

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_ID | MIIM_STRING;
	mii.wID = IDM_BOOKMARKS_BOOKMARKTHISTAB;
	mii.dwTypeData = bookmarkThisTabText.data();
	InsertMenuItem(menu.get(), position++, TRUE, &mii);

	UINT dpi = m_dpiCompat.GetDpiForWindow(m_expp->GetMainWindow());

	ResourceHelper::SetMenuItemImage(menu.get(), IDM_BOOKMARKS_BOOKMARKTHISTAB,
		m_expp->GetIconResourceLoader(), Icon::AddBookmark, dpi, menuImages);

	std::wstring manageBookmarksText = ResourceHelper::LoadString(m_expp->GetLanguageModule() , IDS_MENU_MANAGE_BOOKMARKS);

	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_ID | MIIM_STRING;
	mii.wID = IDM_BOOKMARKS_MANAGEBOOKMARKS;
	mii.dwTypeData = manageBookmarksText.data();
	InsertMenuItem(menu.get(), position++, TRUE, &mii);

	ResourceHelper::SetMenuItemImage(menu.get(), IDM_BOOKMARKS_MANAGEBOOKMARKS,
		m_expp->GetIconResourceLoader(), Icon::Bookmarks, dpi, menuImages);

	const BookmarkItem *bookmarksMenuFolder = m_bookmarkTree->GetBookmarksMenuFolder();

	if (bookmarksMenuFolder->GetChildren().empty())
	{
		return menu;
	}

	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_FTYPE;
	mii.fType = MFT_SEPARATOR;
	InsertMenuItem(menu.get(), position++, TRUE, &mii);

	m_menuBuilder.BuildMenu(menu.get(), bookmarksMenuFolder, m_menuIdRange,
		position, menuItemMappings);

	return menu;
}

void BookmarksMainMenu::OnMenuItemClicked(int menuItemId)
{
	auto itr = m_menuItemMappings.find(menuItemId);

	if (itr == m_menuItemMappings.end())
	{
		return;
	}

	const BookmarkItem *bookmark = itr->second;

	assert(bookmark->IsBookmark());

	Tab &selectedTab = m_expp->GetTabContainer()->GetSelectedTab();
	selectedTab.GetShellBrowser()->GetNavigationController()->BrowseFolder(bookmark->GetLocation());
}