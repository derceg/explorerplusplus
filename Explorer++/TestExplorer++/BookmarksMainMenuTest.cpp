// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/UI/BookmarksMainMenu.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "IconFetcherFake.h"
#include "MainResource.h"
#include "MenuTestHost.h"
#include "ShellBrowser/ShellBrowser.h"
#include <gtest/gtest.h>

using namespace testing;

class BookmarksMainMenuTest : public BrowserTestBase
{
protected:
	BookmarksMainMenuTest() :
		m_browser(AddBrowser()),
		m_tab(m_browser->AddTab(L"c:\\initial\\folder"))
	{
	}

	BrowserWindowFake *const m_browser;
	Tab *const m_tab;

	IconFetcherFake m_iconFetcher;
};

TEST_F(BookmarksMainMenuTest, BookmarkSelection)
{
	auto *bookmark1 = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 1", L"c:\\"));
	auto *bookmark2 = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetOtherBookmarksFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 2", L"d:\\"));

	UINT startId = 1000;
	UINT endId = 2000;

	MenuTestHost menuHost;
	auto *rootMenuView = menuHost.GetView();
	BookmarksMainMenu menu(rootMenuView, &m_acceleratorManager, &m_bookmarkTree, m_browser,
		&m_iconFetcher, &m_platformContext, &m_resourceLoader, startId, endId);

	// The first assigned ID should correspond to the bookmark contained in the Bookmarks Menu
	// folder.
	menuHost.SelectItem(startId, false, false);
	EXPECT_THAT(m_tab->GetShellBrowser()->GetDirectoryPath(), StrCaseEq(bookmark1->GetLocation()));

	auto *otherBookmarksSubMenuView =
		rootMenuView->GetSubMenuViewForTesting(IDM_BOOKMARKS_OTHER_BOOKMARKS_POPUP);
	ASSERT_EQ(otherBookmarksSubMenuView->GetNumItems(), 1);
	menuHost.SelectItem(otherBookmarksSubMenuView->GetItemIdForTesting(0), false, false);
	EXPECT_THAT(m_tab->GetShellBrowser()->GetDirectoryPath(), StrCaseEq(bookmark2->GetLocation()));
}
