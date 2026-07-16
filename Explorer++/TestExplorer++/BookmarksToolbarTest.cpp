// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/UI/BookmarksToolbar.h"
#include "Bookmarks/UI/Views/BookmarksToolbarView.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "CustomFont.h"
#include "FontHelper.h"
#include "IconFetcherFake.h"
#include "PidlTestHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include <boost/range/combine.hpp>
#include <gtest/gtest.h>
#include <uxtheme.h>

class BookmarksToolbarTest : public BrowserTestBase
{
protected:
	BookmarksToolbarTest() :
		m_browser(AddBrowser()),
		m_bookmarksToolbarView(BookmarksToolbarView::Create(m_browser->GetHWND(), &m_config))
	{
		m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
			std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt));
		m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
			std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"c:\\path\\to\\folder"));
		m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
			std::make_unique<BookmarkItem>(std::nullopt, L"Folder 2", std::nullopt));

		m_bookmarksToolbar =
			BookmarksToolbar::Create(m_bookmarksToolbarView, m_browser, &m_acceleratorManager,
				&m_resourceLoader, &m_iconFetcher, &m_bookmarkTree, &m_platformContext);
	}

	void VerifyToolbarButtons()
	{
		const auto &buttons = m_bookmarksToolbarView->GetButtons();
		const auto &bookmarks = m_bookmarkTree.GetBookmarksToolbarFolder()->GetChildren();
		ASSERT_EQ(buttons.size(), bookmarks.size());

		// TODO: This should use std::views::zip once C++23 support is available.
		for (const auto &[button, bookmark] : boost::combine(buttons, bookmarks))
		{
			EXPECT_EQ(button->GetText(), bookmark->GetName());
			EXPECT_EQ(button->GetTooltipText(),
				bookmark->IsBookmark() ? bookmark->GetName() + L"\n" + bookmark->GetLocation()
									   : L"");
		}
	}

	IconFetcherFake m_iconFetcher;

	BrowserWindowFake *const m_browser;
	BookmarksToolbarView *const m_bookmarksToolbarView;
	BookmarksToolbar *m_bookmarksToolbar = nullptr;
};

TEST_F(BookmarksToolbarTest, InitialBookmarks)
{
	VerifyToolbarButtons();
}

TEST_F(BookmarksToolbarTest, AddBookmarks)
{
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 3", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Windows", L"c:\\windows"));

	// These items are being created in a different folder, so shouldn't be added to the toolbar.
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetOtherBookmarksFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetOtherBookmarksFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"d:\\"));

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

	const auto &buttons = m_bookmarksToolbarView->GetButtons();
	const auto &bookmarks = m_bookmarkTree.GetBookmarksToolbarFolder()->GetChildren();
	ASSERT_EQ(buttons.size(), bookmarks.size());

	// TODO: This should use std::views::zip once C++23 support is available.
	for (const auto &[button, bookmark] : boost::combine(buttons, bookmarks))
	{
		if (bookmark->IsFolder())
		{
			continue;
		}

		button->OnClicked(MouseEvent{ { 0, 0 }, false, false });

		auto *currentEntry = tab->GetShellBrowser()->GetNavigationController()->GetCurrentEntry();
		ASSERT_NE(currentEntry, nullptr);
		EXPECT_EQ(currentEntry->GetPidl(), CreateSimplePidlForTest(bookmark->GetLocation()));
	}
}

class BookmarksToolbarFontTest : public BrowserTestBase
{
protected:
	BookmarksToolbarFontTest() :
		m_browser(AddBrowser()),
		m_bookmarksToolbarView(CreateBookmarksToolbarView(m_browser->GetHWND(), &m_config))
	{
		m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
			std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"c:\\path"));

		m_bookmarksToolbar =
			BookmarksToolbar::Create(m_bookmarksToolbarView, m_browser, &m_acceleratorManager,
				&m_resourceLoader, &m_iconFetcher, &m_bookmarkTree, &m_platformContext);
	}

	static BookmarksToolbarView *CreateBookmarksToolbarView(HWND parent, Config *config)
	{
		config->mainFont = CustomFont(L"Segoe UI", 15);
		return BookmarksToolbarView::Create(parent, config);
	}

	IconFetcherFake m_iconFetcher;
	BrowserWindowFake *const m_browser;
	BookmarksToolbarView *const m_bookmarksToolbarView;
	BookmarksToolbar *m_bookmarksToolbar = nullptr;
};

TEST_F(BookmarksToolbarFontTest, UsesConfiguredFontOnStartup)
{
	// The theme is applied after the main window and all its child controls have been created.
	ASSERT_TRUE(SUCCEEDED(SetWindowTheme(m_bookmarksToolbarView->GetHWND(), L"", nullptr)));

	auto toolbarFont = reinterpret_cast<HFONT>(
		SendMessage(m_bookmarksToolbarView->GetHWND(), WM_GETFONT, 0, 0));
	ASSERT_NE(toolbarFont, nullptr);

	LOGFONT toolbarLogFont;
	ASSERT_EQ(GetObject(toolbarFont, sizeof(toolbarLogFont), &toolbarLogFont),
		static_cast<int>(sizeof(toolbarLogFont)));

	auto expectedFont = CreateFontFromNameAndSize(L"Segoe UI", 15,
		m_bookmarksToolbarView->GetHWND());
	ASSERT_NE(expectedFont, nullptr);

	LOGFONT expectedLogFont;
	ASSERT_EQ(GetObject(expectedFont.get(), sizeof(expectedLogFont), &expectedLogFont),
		static_cast<int>(sizeof(expectedLogFont)));

	EXPECT_EQ(toolbarLogFont, expectedLogFont);
}
