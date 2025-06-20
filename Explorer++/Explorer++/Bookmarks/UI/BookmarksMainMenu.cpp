// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarksMainMenu.h"
#include "AcceleratorHelper.h"
#include "App.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkTree.h"
#include "BrowserWindow.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ResourceLoader.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/MenuHelper.h"

BookmarksMainMenu::BookmarksMainMenu(App *app, BrowserWindow *browserWindow,
	CoreInterface *coreInterface, const ResourceLoader *resourceLoader, IconFetcher *iconFetcher,
	BookmarkTree *bookmarkTree, const BookmarkMenuBuilder::MenuIdRange &menuIdRange) :
	m_app(app),
	m_coreInterface(coreInterface),
	m_resourceLoader(resourceLoader),
	m_bookmarkTree(bookmarkTree),
	m_menuIdRange(menuIdRange),
	m_menuBuilder(resourceLoader, iconFetcher),
	m_controller(bookmarkTree, browserWindow, app->GetAcceleratorManager(),
		app->GetResourceLoader(), coreInterface->GetMainWindow(), app->GetClipboardStore())
{
	m_connections.push_back(coreInterface->AddMainMenuPreShowObserver(
		std::bind_front(&BookmarksMainMenu::OnMainMenuPreShow, this)));
	m_connections.push_back(browserWindow->AddMenuHelpTextRequestObserver(
		std::bind_front(&BookmarksMainMenu::MaybeGetMenuHelpText, this)));
	m_connections.push_back(coreInterface->AddMainMenuItemMiddleClickedObserver(
		std::bind_front(&BookmarksMainMenu::OnMenuItemMiddleClicked, this)));
	m_connections.push_back(coreInterface->AddMainMenuItemRightClickedObserver(
		std::bind_front(&BookmarksMainMenu::OnMenuItemRightClicked, this)));
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
	menuInfo.nextMenuId = m_menuIdRange.startId;
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

	std::wstring bookmarkThisTabText = m_resourceLoader->LoadString(IDS_MENU_BOOKMARK_THIS_TAB);
	MenuHelper::AddStringItem(menu.get(), IDM_BOOKMARKS_BOOKMARKTHISTAB, bookmarkThisTabText, 0,
		TRUE);
	ResourceHelper::SetMenuItemImage(menu.get(), IDM_BOOKMARKS_BOOKMARKTHISTAB, m_resourceLoader,
		Icon::AddBookmark, dpi, menuImages);

	std::wstring bookmarkAllTabsText = m_resourceLoader->LoadString(IDS_MENU_BOOKMARK_ALL_TABS);
	MenuHelper::AddStringItem(menu.get(), IDM_BOOKMARKS_BOOKMARK_ALL_TABS, bookmarkAllTabsText, 1,
		TRUE);

	std::wstring manageBookmarksText = m_resourceLoader->LoadString(IDS_MENU_MANAGE_BOOKMARKS);
	MenuHelper::AddStringItem(menu.get(), IDM_BOOKMARKS_MANAGEBOOKMARKS, manageBookmarksText, 2,
		TRUE);
	ResourceHelper::SetMenuItemImage(menu.get(), IDM_BOOKMARKS_MANAGEBOOKMARKS, m_resourceLoader,
		Icon::Bookmarks, dpi, menuImages);

	AddBookmarkItemsToMenu(menu.get(), m_menuIdRange, GetMenuItemCount(menu.get()), menuImages,
		menuInfo);
	AddOtherBookmarksToMenu(menu.get(), { menuInfo.nextMenuId, m_menuIdRange.endId },
		GetMenuItemCount(menu.get()), menuImages, menuInfo);

	UpdateMenuAcceleratorStrings(menu.get(), m_app->GetAcceleratorManager());

	return menu;
}

void BookmarksMainMenu::AddBookmarkItemsToMenu(HMENU menu,
	const BookmarkMenuBuilder::MenuIdRange &menuIdRange, int position,
	std::vector<wil::unique_hbitmap> &menuImages, BookmarkMenuBuilder::MenuInfo &menuInfo)
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

void BookmarksMainMenu::AddOtherBookmarksToMenu(HMENU menu,
	const BookmarkMenuBuilder::MenuIdRange &menuIdRange, int position,
	std::vector<wil::unique_hbitmap> &menuImages, BookmarkMenuBuilder::MenuInfo &menuInfo)
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

	auto otherBookmarksId = menuInfo.nextMenuId++;
	MenuHelper::AddSubMenuItem(menu, otherBookmarksId, otherBookmarksFolder->GetName(),
		std::move(subMenu), position++, TRUE);
	menuInfo.itemIdMap.insert({ otherBookmarksId,
		{ otherBookmarksFolder, BookmarkMenuBuilder::MenuItemType::BookmarkItem } });
}

std::optional<std::wstring> BookmarksMainMenu::MaybeGetMenuHelpText(HMENU menu, UINT id)
{
	if (!MenuHelper::IsPartOfMenu(m_bookmarksMenu.get(), menu))
	{
		return std::nullopt;
	}

	auto itr = m_menuInfo.itemIdMap.find(id);

	if (itr == m_menuInfo.itemIdMap.end())
	{
		return std::nullopt;
	}

	const BookmarkItem *bookmark = itr->second.bookmarkItem;
	return bookmark->GetLocation();
}

void BookmarksMainMenu::OnMenuItemClicked(UINT menuItemId)
{
	auto itr = m_menuInfo.itemIdMap.find(menuItemId);

	if (itr == m_menuInfo.itemIdMap.end())
	{
		return;
	}

	m_controller.OnMenuItemSelected(itr->second.bookmarkItem, IsKeyDown(VK_CONTROL),
		IsKeyDown(VK_SHIFT));
}

bool BookmarksMainMenu::OnMenuItemMiddleClicked(const POINT &pt, bool isCtrlKeyDown,
	bool isShiftKeyDown)
{
	auto menuItemId = MenuHelper::MaybeGetMenuItemAtPoint(m_bookmarksMenu.get(), pt);

	if (!menuItemId)
	{
		return false;
	}

	auto itr = m_menuInfo.itemIdMap.find(*menuItemId);

	if (itr == m_menuInfo.itemIdMap.end())
	{
		// The item can be one of the other existing (non-bookmark) menu items.
		return true;
	}

	if (!MenuHelper::IsMenuItemEnabled(m_bookmarksMenu.get(), *menuItemId, false))
	{
		return true;
	}

	m_controller.OnMenuItemMiddleClicked(itr->second.bookmarkItem, isCtrlKeyDown, isShiftKeyDown);

	return true;
}

bool BookmarksMainMenu::OnMenuItemRightClicked(HMENU menu, int index, const POINT &pt)
{
	if (!MenuHelper::IsPartOfMenu(m_bookmarksMenu.get(), menu))
	{
		return false;
	}

	auto menuItemId = MenuHelper::GetMenuItemIDIncludingSubmenu(menu, index);
	auto itr = m_menuInfo.itemIdMap.find(menuItemId);

	if (itr == m_menuInfo.itemIdMap.end())
	{
		// It's valid for the item not to be found, as the bookmarks menu contains several existing
		// menu items and this class only manages the actual bookmark items on the menu.
		return false;
	}

	m_controller.OnMenuItemRightClicked(itr->second.bookmarkItem, pt);

	return true;
}
