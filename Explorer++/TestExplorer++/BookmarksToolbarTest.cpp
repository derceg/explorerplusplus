// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/UI/BookmarksToolbar.h"
#include "Bookmarks/UI/Views/BookmarksToolbarView.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "IconFetcherFake.h"
#include "PidlTestHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include <boost/range/combine.hpp>
#include <gtest/gtest.h>
#include <memory>

class BookmarksToolbarTest : public BrowserTestBase
{
protected:
	BookmarksToolbarTest() :
		m_browser(AddBrowser()),
		m_bookmarksToolbarView(m_browser->GetView(), m_browser, &m_config)
	{
		m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
			std::make_unique<BookmarkItem>(L"Folder 1"));
		m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
			std::make_unique<BookmarkItem>(L"Bookmark", L"c:\\path\\to\\folder"));
		m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
			std::make_unique<BookmarkItem>(L"Folder 2"));

		m_bookmarksToolbar = std::make_unique<BookmarksToolbar>(&m_bookmarksToolbarView, m_browser,
			&m_acceleratorManager, &m_resourceLoader, &m_iconFetcher, &m_bookmarkTree,
			&m_platformContext);
	}

	void VerifyToolbarButtons()
	{
		auto toolbarItems = m_bookmarksToolbarView.GetItems();
		const auto &bookmarks = m_bookmarkTree.GetBookmarksToolbarFolder()->GetChildren();
		ASSERT_EQ(toolbarItems.size(), bookmarks.size());

		// TODO: This should use std::views::zip once C++23 support is available.
		for (const auto &[toolbarItem, bookmark] : boost::combine(toolbarItems, bookmarks))
		{
			ASSERT_TRUE(toolbarItem.IsButton());

			auto *button = toolbarItem.GetButton();
			EXPECT_EQ(button->GetText(), bookmark->GetName());
			EXPECT_EQ(button->GetTooltipText(),
				bookmark->IsBookmark() ? bookmark->GetName() + L"\n" + bookmark->GetLocation()
									   : L"");
		}
	}

	IconFetcherFake m_iconFetcher;

	BrowserWindowFake *const m_browser;
	BookmarksToolbarView m_bookmarksToolbarView;
	std::unique_ptr<BookmarksToolbar> m_bookmarksToolbar;
};

TEST_F(BookmarksToolbarTest, InitialBookmarks)
{
	VerifyToolbarButtons();
}

TEST_F(BookmarksToolbarTest, AddBookmarks)
{
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
		std::make_unique<BookmarkItem>(L"Folder 3"));
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
		std::make_unique<BookmarkItem>(L"Windows", L"c:\\windows"));

	// These items are being created in a different folder, so shouldn't be added to the toolbar.
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetOtherBookmarksFolder(),
		std::make_unique<BookmarkItem>(L"Folder"));
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetOtherBookmarksFolder(),
		std::make_unique<BookmarkItem>(L"Bookmark", L"d:\\"));

	VerifyToolbarButtons();
}

TEST_F(BookmarksToolbarTest, UpdateBookmarks)
{
	const auto &bookmarks = m_bookmarkTree.GetBookmarksToolbarFolder()->GetChildren();
	bookmarks[0]->SetName(L"Updated folder name");
	bookmarks[1]->SetName(L"Updated bookmark name");

	VerifyToolbarButtons();
}

TEST_F(BookmarksToolbarTest, MoveBookmarks)
{
	auto *bookmarksToolbarFolder = m_bookmarkTree.GetBookmarksToolbarFolder();
	m_bookmarkTree.MoveBookmarkItem(bookmarksToolbarFolder->GetChildren()[0].get(),
		bookmarksToolbarFolder, 2);

	m_bookmarkTree.MoveBookmarkItem(bookmarksToolbarFolder->GetChildren()[1].get(),
		m_bookmarkTree.GetOtherBookmarksFolder(), 0);

	VerifyToolbarButtons();
}

TEST_F(BookmarksToolbarTest, RemoveBookmarks)
{
	std::vector<BookmarkItem *> childBookmarks;

	for (const auto &bookmark : m_bookmarkTree.GetBookmarksToolbarFolder()->GetChildren())
	{
		childBookmarks.push_back(bookmark.get());
	}

	for (auto *childBookmark : childBookmarks)
	{
		m_bookmarkTree.RemoveBookmarkItem(childBookmark);
	}

	VerifyToolbarButtons();
}

TEST_F(BookmarksToolbarTest, OpenBookmarkOnClick)
{
	auto *tab = m_browser->AddTab(L"c:\\original\\path");

	auto toolbarItems = m_bookmarksToolbarView.GetItems();
	const auto &bookmarks = m_bookmarkTree.GetBookmarksToolbarFolder()->GetChildren();
	ASSERT_EQ(toolbarItems.size(), bookmarks.size());

	// TODO: This should use std::views::zip once C++23 support is available.
	for (const auto &[toolbarItem, bookmark] : boost::combine(toolbarItems, bookmarks))
	{
		if (bookmark->IsFolder())
		{
			continue;
		}

		ASSERT_TRUE(toolbarItem.IsButton());

		auto *button = toolbarItem.GetButton();
		button->OnClicked(MouseEvent{ { 0, 0 }, false, false });

		auto *currentEntry = tab->GetShellBrowser()->GetNavigationController()->GetCurrentEntry();
		ASSERT_NE(currentEntry, nullptr);
		EXPECT_EQ(currentEntry->GetPidl(), CreateSimplePidlForTest(bookmark->GetLocation()));
	}
}
