// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/UI/BookmarkContextMenu.h"
#include "Bookmarks/BookmarkClipboard.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "CopiedBookmark.h"
#include "MainResource.h"
#include "MenuViewFake.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellTestHelper.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class BookmarkContextMenuTestBase : public BrowserTestBase
{
protected:
	BookmarkContextMenuTestBase() :
		m_browser(AddBrowser()),
		m_tab(m_browser->AddTab(L"c:\\original\\path"))
	{
	}

	BrowserWindowFake *const m_browser;
	Tab *const m_tab;
};

class BookmarkContextMenuSingleBookmarkTest : public BookmarkContextMenuTestBase
{
protected:
	BookmarkContextMenuSingleBookmarkTest() :
		m_bookmark(m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
			std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"c:\\bookmarked\\folder"),
			0))
	{
	}

	std::unique_ptr<BookmarkContextMenu> BuildContextMenu(MenuView *menuView)
	{
		return std::make_unique<BookmarkContextMenu>(menuView, &m_acceleratorManager,
			&m_bookmarkTree, RawBookmarkItems{ m_bookmark }, &m_resourceLoader, m_browser,
			m_browser->GetHWND(), &m_platformContext);
	}

	BookmarkItem *const m_bookmark;
};

TEST_F(BookmarkContextMenuSingleBookmarkTest, Open)
{
	MenuViewFake menuView;
	auto contextMenu = BuildContextMenu(&menuView);

	menuView.SelectItem(IDM_BOOKMARK_CONTEXT_MENU_OPEN, false, false);

	EXPECT_EQ(m_tab->GetShellBrowser()->GetNavigationController()->GetNumHistoryEntries(), 2);

	EXPECT_EQ(m_tab->GetShellBrowser()->GetDirectory(),
		CreateSimplePidlForTest(m_bookmark->GetLocation()));
}

TEST_F(BookmarkContextMenuSingleBookmarkTest, OpenInNewTab)
{
	MenuViewFake menuView;
	auto contextMenu = BuildContextMenu(&menuView);

	menuView.SelectItem(IDM_BOOKMARK_CONTEXT_MENU_OPEN_IN_NEW_TAB, false, false);

	auto *tabContainer = m_browser->GetActiveTabContainer();
	ASSERT_EQ(tabContainer->GetNumTabs(), 2);

	const auto &tab = tabContainer->GetTabByIndex(1);
	EXPECT_EQ(tab.GetShellBrowser()->GetDirectory(),
		CreateSimplePidlForTest(m_bookmark->GetLocation()));
}

TEST_F(BookmarkContextMenuSingleBookmarkTest, Cut)
{
	MenuViewFake menuView;
	auto contextMenu = BuildContextMenu(&menuView);

	CopiedBookmark copiedBookmark(*m_bookmark);
	const auto *parentFolder = m_bookmark->GetParent();

	menuView.SelectItem(IDM_BOOKMARK_CONTEXT_MENU_CUT, false, false);

	// Cutting the bookmark should have removed it from the tree.
	ASSERT_TRUE(parentFolder->GetChildren().empty());

	BookmarkClipboard bookmarkClipboard(m_platformContext.GetClipboardStore());
	auto clipboardItems = bookmarkClipboard.ReadBookmarks();
	EXPECT_THAT(clipboardItems, ElementsAre(Pointee(copiedBookmark)));
}

TEST_F(BookmarkContextMenuSingleBookmarkTest, Copy)
{
	MenuViewFake menuView;
	auto contextMenu = BuildContextMenu(&menuView);

	CopiedBookmark copiedBookmark(*m_bookmark);

	menuView.SelectItem(IDM_BOOKMARK_CONTEXT_MENU_COPY, false, false);

	BookmarkClipboard bookmarkClipboard(m_platformContext.GetClipboardStore());
	auto clipboardItems = bookmarkClipboard.ReadBookmarks();
	EXPECT_THAT(clipboardItems, ElementsAre(Pointee(copiedBookmark)));
}

TEST_F(BookmarkContextMenuSingleBookmarkTest, Paste)
{
	// This bookmark is explicitly copied before the menu is constructed. That's because the paste
	// menu item will be disabled if there are no bookmarks on the clipboard, which will then cause
	// the test to fail. Copying the bookmark first ensures that the paste item will be enabled when
	// the menu is built.
	auto *bookmark = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetOtherBookmarksFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"f:\\"), 0);
	CopiedBookmark copiedBookmark(*bookmark);
	ASSERT_TRUE(BookmarkHelper::CopyBookmarkItems(m_platformContext.GetClipboardStore(),
		&m_bookmarkTree, { bookmark }, ClipboardAction::Copy));

	MenuViewFake menuView;
	auto contextMenu = BuildContextMenu(&menuView);

	menuView.SelectItem(IDM_BOOKMARK_CONTEXT_MENU_PASTE, false, false);

	const auto *parentFolder = m_bookmark->GetParent();
	ASSERT_EQ(parentFolder->GetChildren().size(), 2u);
	EXPECT_THAT(parentFolder->GetChildren()[1], Pointee(copiedBookmark));
}

TEST_F(BookmarkContextMenuSingleBookmarkTest, Delete)
{
	MenuViewFake menuView;
	auto contextMenu = BuildContextMenu(&menuView);

	MockFunction<void(const std::wstring &guid)> removedCallback;
	m_bookmarkTree.bookmarkItemRemovedSignal.AddObserver(removedCallback.AsStdFunction());

	EXPECT_CALL(removedCallback, Call(m_bookmark->GetGUID()));
	menuView.SelectItem(IDM_BOOKMARK_CONTEXT_MENU_DELETE, false, false);
}

class BookmarkContextMenuMultipleBookmarksTest : public BookmarkContextMenuTestBase
{
protected:
	BookmarkContextMenuMultipleBookmarksTest()
	{
		m_bookmarkItems.push_back(m_bookmarkTree.AddBookmarkItem(
			m_bookmarkTree.GetBookmarksToolbarFolder(),
			std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 1", L"c:\\bookmarked\\folder"),
			0));
		m_bookmarkItems.push_back(
			m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
				std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 2", L"d:\\projects"), 1));
		m_bookmarkItems.push_back(
			m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
				std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 3", L"e:\\documents"), 2));
	}

	std::unique_ptr<BookmarkContextMenu> BuildContextMenu(MenuView *menuView)
	{
		return std::make_unique<BookmarkContextMenu>(menuView, &m_acceleratorManager,
			&m_bookmarkTree, m_bookmarkItems, &m_resourceLoader, m_browser, m_browser->GetHWND(),
			&m_platformContext);
	}

	RawBookmarkItems m_bookmarkItems;
};

TEST_F(BookmarkContextMenuMultipleBookmarksTest, OpenAll)
{
	MenuViewFake menuView;
	auto contextMenu = BuildContextMenu(&menuView);

	menuView.SelectItem(IDM_BOOKMARK_CONTEXT_MENU_OPEN_ALL, false, false);

	auto *tabContainer = m_browser->GetActiveTabContainer();
	int numBookmarks = static_cast<int>(m_bookmarkItems.size());
	ASSERT_EQ(tabContainer->GetNumTabs(), numBookmarks + 1);

	for (int i = 0; i < numBookmarks; i++)
	{
		const auto &tab = tabContainer->GetTabByIndex(i + 1);
		EXPECT_EQ(tab.GetShellBrowser()->GetDirectory(),
			CreateSimplePidlForTest(m_bookmarkItems[i]->GetLocation()));
	}
}
