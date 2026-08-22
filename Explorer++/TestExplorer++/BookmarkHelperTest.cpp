// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/BookmarkHelper.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "ShellBrowser/ShellBrowser.h"
#include "../Helper/ShellHelper.h"
#include <boost/range/combine.hpp>
#include <gtest/gtest.h>

class BookmarkHelperTest : public BrowserTestBase
{
};

TEST_F(BookmarkHelperTest, AddBrowserTabsToBookmarkFolder)
{
	auto *browser = AddBrowser();
	browser->AddTab(L"c:\\path\\to\\item");
	browser->AddTab(L"h:\\");
	browser->AddTab(L"z:\\folder");

	auto *parentFolder = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetOtherBookmarksFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Parent folder", std::nullopt));
	BookmarkHelper::AddBrowserTabsToBookmarkFolder(&m_bookmarkTree, browser, parentFolder);

	const auto &bookmarks = parentFolder->GetChildren();
	const auto tabs = browser->GetActiveTabContainer()->GetAllTabsInOrder();
	ASSERT_EQ(bookmarks.size(), tabs.size());

	// TODO: This should use std::views::zip once C++23 support is available.
	for (const auto &[bookmark, tab] : boost::combine(bookmarks, tabs))
	{
		EXPECT_TRUE(bookmark->IsBookmark());

		std::wstring expectedName;
		ASSERT_HRESULT_SUCCEEDED(GetDisplayName(tab->GetShellBrowser()->GetDirectory().Raw(),
			SHGDN_INFOLDER, expectedName));
		EXPECT_EQ(bookmark->GetName(), expectedName);

		EXPECT_EQ(bookmark->GetLocation(), tab->GetShellBrowser()->GetDirectoryPath());
	}
}
