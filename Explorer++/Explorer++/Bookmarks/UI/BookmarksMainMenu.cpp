// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarksMainMenu.h"
#include "Bookmarks/BookmarkIconManager.h"
#include "Bookmarks/BookmarkTree.h"
#include "Bookmarks/UI/BookmarksMenu.h"
#include "BrowserWindow.h"
#include "MainResource.h"
#include "MenuView.h"
#include "ResourceIconModel.h"
#include "ResourceLoader.h"
#include "../Helper/DpiCompatibility.h"

BookmarksMainMenu::BookmarksMainMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, BookmarkTree *bookmarkTree,
	BrowserWindow *browser, IconFetcher *iconFetcher, PlatformContext *platformContext,
	const ResourceLoader *resourceLoader, UINT startId, UINT endId) :
	MenuBase(menuView, acceleratorManager, startId, endId),
	m_bookmarkTree(bookmarkTree),
	m_browser(browser),
	m_platformContext(platformContext),
	m_resourceLoader(resourceLoader),
	m_defaultDpiIconSize(DpiCompatibility::GetInstance().GetSystemMetricsForDpi(SM_CXSMICON,
		USER_DEFAULT_SCREEN_DPI))
{
	auto &dpiCompat = DpiCompatibility::GetInstance();
	int scaledIconSize = dpiCompat.ScaleValue(m_browser->GetHWND(), m_defaultDpiIconSize);
	m_iconManager = std::make_unique<BookmarkIconManager>(resourceLoader, iconFetcher,
		scaledIconSize, scaledIconSize);

	// Note that this class builds the menu, but item selection is currently handled externally, by
	// the main window switch.
	BuildMenu();

	// The observers set up here will result in the menu being rebuilt any time a bookmark item
	// changes. That's somewhat broader then necessary, as only changes in Bookmarks Menu/Other
	// Bookmarks affect the menu.
	m_connections.push_back(bookmarkTree->bookmarkItemAddedSignal.AddObserver(
		std::bind(&BookmarksMainMenu::RebuildMenu, this)));
	m_connections.push_back(bookmarkTree->bookmarkItemUpdatedSignal.AddObserver(
		std::bind(&BookmarksMainMenu::RebuildMenu, this)));
	m_connections.push_back(bookmarkTree->bookmarkItemMovedSignal.AddObserver(
		std::bind(&BookmarksMainMenu::RebuildMenu, this)));
	m_connections.push_back(bookmarkTree->bookmarkItemRemovedSignal.AddObserver(
		std::bind(&BookmarksMainMenu::RebuildMenu, this)));
}

BookmarksMainMenu::~BookmarksMainMenu() = default;

void BookmarksMainMenu::RebuildMenu()
{
	m_menuView->ClearMenu();
	m_bookmarksMenuContents.reset();
	m_otherBookmarksMenuContents.reset();
	BuildMenu();
}

void BookmarksMainMenu::BuildMenu()
{
	m_menuView->AppendItem(nullptr, IDM_BOOKMARKS_BOOKMARK_THIS_TAB,
		m_resourceLoader->LoadString(IDS_BOOKMARKS_BOOKMARK_THIS_TAB),
		std::make_unique<ResourceIconModel>(Icon::AddBookmark, m_defaultDpiIconSize,
			m_resourceLoader),
		m_resourceLoader->LoadString(IDS_BOOKMARKS_BOOKMARK_THIS_TAB_HELP_TEXT),
		GetAcceleratorTextForId(IDM_BOOKMARKS_BOOKMARK_THIS_TAB));

	m_menuView->AppendItem(nullptr, IDM_BOOKMARKS_BOOKMARK_ALL_TABS,
		m_resourceLoader->LoadString(IDS_BOOKMARKS_BOOKMARK_ALL_TABS), {},
		m_resourceLoader->LoadString(IDS_BOOKMARKS_BOOKMARK_ALL_TABS_HELP_TEXT),
		GetAcceleratorTextForId(IDM_BOOKMARKS_BOOKMARK_ALL_TABS));

	m_menuView->AppendItem(nullptr, IDM_BOOKMARKS_MANAGE_BOOKMARKS,
		m_resourceLoader->LoadString(IDS_BOOKMARKS_MANAGE_BOOKMARKS),
		std::make_unique<ResourceIconModel>(Icon::Bookmarks, m_defaultDpiIconSize,
			m_resourceLoader),
		m_resourceLoader->LoadString(IDS_BOOKMARKS_MANAGE_BOOKMARKS_HELP_TEXT),
		GetAcceleratorTextForId(IDM_BOOKMARKS_MANAGE_BOOKMARKS));

	auto *bookmarksMenuFolder = m_bookmarkTree->GetBookmarksMenuFolder();
	UINT startId = GetIdRange().startId;
	UINT endId = GetIdRange().endId;

	if (!bookmarksMenuFolder->GetChildren().empty())
	{
		m_menuView->AppendSeparator();

		m_bookmarksMenuContents = std::make_unique<BookmarksMenu>(m_menuView, m_acceleratorManager,
			m_bookmarkTree, bookmarksMenuFolder, m_iconManager.get(), m_browser,
			m_browser->GetHWND(), m_platformContext, m_resourceLoader, nullptr, startId, endId);

		startId = m_bookmarksMenuContents->GetNextId();
	}

	auto *otherBookmarksFolder = m_bookmarkTree->GetOtherBookmarksFolder();

	if (!otherBookmarksFolder->GetChildren().empty())
	{
		m_menuView->AppendSeparator();

		auto *otherBookmarksMenu = m_menuView->AppendSubMenu(nullptr,
			IDM_BOOKMARKS_OTHER_BOOKMARKS_POPUP, otherBookmarksFolder->GetName());

		m_otherBookmarksMenuContents =
			std::make_unique<BookmarksMenu>(otherBookmarksMenu, m_acceleratorManager,
				m_bookmarkTree, otherBookmarksFolder, m_iconManager.get(), m_browser,
				m_browser->GetHWND(), m_platformContext, m_resourceLoader, nullptr, startId, endId);
	}
}
