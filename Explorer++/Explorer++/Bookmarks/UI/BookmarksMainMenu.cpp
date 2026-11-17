// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarksMainMenu.h"
#include "AppServices.h"
#include "Bookmarks/BookmarkIconManager.h"
#include "Bookmarks/BookmarkTree.h"
#include "Bookmarks/UI/BookmarksMenu.h"
#include "BrowserWindow.h"
#include "MainResource.h"
#include "MenuView.h"
#include "ResourceIconModel.h"
#include "ResourceLoader.h"
#include "../Helper/DpiCompatibility.h"

BookmarksMainMenu::BookmarksMainMenu(MenuView *menuView, BrowserWindow *browser,
	AppServices *appServices) :
	MenuBase(menuView, appServices->GetAcceleratorManager()),
	m_bookmarkTree(appServices->GetBookmarkTree()),
	m_browser(browser),
	m_platformContext(appServices->GetPlatformContext()),
	m_resourceLoader(appServices->GetResourceLoader())
{
	auto &dpiCompat = DpiCompatibility::GetInstance();
	int scaledIconSize =
		dpiCompat.ScaleValue(m_browser->GetHWND(), GetIconPixelSizeAtDefaultDpi(IconSize::Small));
	m_iconManager = std::make_unique<BookmarkIconManager>(appServices->GetRuntime(),
		appServices->GetIconFetcher(), m_resourceLoader, scaledIconSize, scaledIconSize);

	// Note that this class builds the menu, but item selection is currently handled externally, by
	// the main window switch.
	BuildMenu();

	// The observers set up here will result in the menu being rebuilt any time a bookmark item
	// changes. That's somewhat broader then necessary, as only changes in Bookmarks Menu/Other
	// Bookmarks affect the menu.
	m_connections.push_back(m_bookmarkTree->bookmarkItemAddedSignal.AddObserver(
		std::bind(&BookmarksMainMenu::RebuildMenu, this)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemUpdatedSignal.AddObserver(
		std::bind(&BookmarksMainMenu::RebuildMenu, this)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemMovedSignal.AddObserver(
		std::bind(&BookmarksMainMenu::RebuildMenu, this)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemRemovedSignal.AddObserver(
		std::bind(&BookmarksMainMenu::RebuildMenu, this)));
}

BookmarksMainMenu::~BookmarksMainMenu() = default;

void BookmarksMainMenu::RebuildMenu()
{
	m_rootMenuView->ClearMenu();
	m_childMenus.clear();
	BuildMenu();
}

void BookmarksMainMenu::BuildMenu()
{
	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_BOOKMARKS_BOOKMARK_THIS_TAB))
			.SetIconModel(std::make_unique<ResourceIconModel>(Icon::AddBookmark, IconSize::Small,
				m_resourceLoader))
			.SetHelpText(m_resourceLoader->LoadString(IDS_BOOKMARKS_BOOKMARK_THIS_TAB_HELP_TEXT))
			.SetAcceleratorText(GetAcceleratorTextForId(IDM_BOOKMARKS_BOOKMARK_THIS_TAB))
			.SetSelectedCallback(
				[this] { SendCommandToBrowser(IDM_BOOKMARKS_BOOKMARK_THIS_TAB); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_BOOKMARKS_BOOKMARK_ALL_TABS))
			.SetHelpText(m_resourceLoader->LoadString(IDS_BOOKMARKS_BOOKMARK_ALL_TABS_HELP_TEXT))
			.SetAcceleratorText(GetAcceleratorTextForId(IDM_BOOKMARKS_BOOKMARK_ALL_TABS))
			.SetSelectedCallback(
				[this] { SendCommandToBrowser(IDM_BOOKMARKS_BOOKMARK_ALL_TABS); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_BOOKMARKS_MANAGE_BOOKMARKS))
			.SetIconModel(std::make_unique<ResourceIconModel>(Icon::Bookmarks, IconSize::Small,
				m_resourceLoader))
			.SetHelpText(m_resourceLoader->LoadString(IDS_BOOKMARKS_MANAGE_BOOKMARKS_HELP_TEXT))
			.SetAcceleratorText(GetAcceleratorTextForId(IDM_BOOKMARKS_MANAGE_BOOKMARKS))
			.SetSelectedCallback([this] { SendCommandToBrowser(IDM_BOOKMARKS_MANAGE_BOOKMARKS); }));

	auto *bookmarksMenuFolder = m_bookmarkTree->GetBookmarksMenuFolder();

	if (!bookmarksMenuFolder->GetChildren().empty())
	{
		m_rootMenuView->AppendSeparator();

		auto bookmarksMenuFolderMenu = std::make_unique<BookmarksMenu>(m_rootMenuView,
			m_acceleratorManager, m_bookmarkTree, bookmarksMenuFolder, m_iconManager.get(),
			m_browser, m_platformContext, m_resourceLoader);

		m_childMenus.push_back(std::move(bookmarksMenuFolderMenu));
	}

	auto *otherBookmarksFolder = m_bookmarkTree->GetOtherBookmarksFolder();

	if (!otherBookmarksFolder->GetChildren().empty())
	{
		m_rootMenuView->AppendSeparator();

		m_childMenus.push_back(
			std::make_unique<BookmarksMenu>(m_rootMenuView, m_acceleratorManager, m_bookmarkTree,
				m_bookmarkTree->GetRoot(), m_iconManager.get(), m_browser, m_platformContext,
				m_resourceLoader, [otherBookmarksFolder](const BookmarkItem *bookmarkItem)
				{ return bookmarkItem == otherBookmarksFolder; }));
	}
}

void BookmarksMainMenu::SendCommandToBrowser(WORD id)
{
	SendMessage(m_browser->GetHWND(), WM_COMMAND, MAKEWPARAM(id, 0), 0);
}
