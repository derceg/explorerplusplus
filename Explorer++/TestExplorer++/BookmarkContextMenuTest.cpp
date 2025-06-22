// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/UI/BookmarkContextMenu.h"
#include "AcceleratorManager.h"
#include "Bookmarks/BookmarkClipboard.h"
#include "Bookmarks/BookmarkTree.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "MainResource.h"
#include "MenuViewFake.h"
#include "ResourceLoaderFake.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellTestHelper.h"
#include "SimulatedClipboardStore.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class BookmarkContextMenuTestBase : public BrowserTestBase
{
protected:
	struct CopiedBookmark
	{
		CopiedBookmark(const BookmarkItem *bookmark) :
			type(bookmark->GetType()),
			guid(bookmark->GetGUID()),
			name(bookmark->GetName()),
			location(bookmark->GetLocation())
		{
		}

		BookmarkItem::Type type;
		std::wstring guid;
		std::wstring name;
		std::wstring location;
	};

	BookmarkContextMenuTestBase() :
		m_browser(AddBrowser()),
		m_tab(m_browser->AddTab(L"c:\\original\\path"))
	{
	}

	void ExpectClipboardBookmarkMatchesCopiedBookmark(const BookmarkItem *clipboardBookmark,
		const CopiedBookmark *copiedBookmark)
	{
		EXPECT_EQ(clipboardBookmark->GetType(), copiedBookmark->type);
		EXPECT_NE(clipboardBookmark->GetGUID(), copiedBookmark->guid);
		EXPECT_EQ(clipboardBookmark->GetName(), copiedBookmark->name);
		EXPECT_EQ(clipboardBookmark->GetLocation(), copiedBookmark->location);
	}

	AcceleratorManager m_acceleratorManager;
	ResourceLoaderFake m_resourceLoader;
	SimulatedClipboardStore m_clipboardStore;

	BookmarkTree m_bookmarkTree;

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
			m_browser->GetHWND(), &m_clipboardStore);
	}

	BookmarkItem *const m_bookmark;
};

TEST_F(BookmarkContextMenuSingleBookmarkTest, Open)
{
	MenuViewFake menuView;
	auto contextMenu = BuildContextMenu(&menuView);

	menuView.SelectItem(IDM_BOOKMARK_CONTEXT_MENU_OPEN, false, false);

	EXPECT_EQ(m_tab->GetShellBrowser()->GetNavigationController()->GetNumHistoryEntries(), 2);

	const auto *currentEntry =
		m_tab->GetShellBrowser()->GetNavigationController()->GetCurrentEntry();
	ASSERT_NE(currentEntry, nullptr);
	EXPECT_EQ(currentEntry->GetPidl(), CreateSimplePidlForTest(m_bookmark->GetLocation()));
}

TEST_F(BookmarkContextMenuSingleBookmarkTest, Cut)
{
	MenuViewFake menuView;
	auto contextMenu = BuildContextMenu(&menuView);

	CopiedBookmark copiedBookmark(m_bookmark);
	const auto *parentFolder = m_bookmark->GetParent();

	menuView.SelectItem(IDM_BOOKMARK_CONTEXT_MENU_CUT, false, false);

	// Cutting the bookmark should have removed it from the tree.
	ASSERT_TRUE(parentFolder->GetChildren().empty());

	BookmarkClipboard bookmarkClipboard(&m_clipboardStore);
	auto clipboardItems = bookmarkClipboard.ReadBookmarks();
	ASSERT_EQ(clipboardItems.size(), 1u);

	ExpectClipboardBookmarkMatchesCopiedBookmark(clipboardItems[0].get(), &copiedBookmark);
}

TEST_F(BookmarkContextMenuSingleBookmarkTest, Copy)
{
	MenuViewFake menuView;
	auto contextMenu = BuildContextMenu(&menuView);

	CopiedBookmark copiedBookmark(m_bookmark);

	menuView.SelectItem(IDM_BOOKMARK_CONTEXT_MENU_COPY, false, false);

	BookmarkClipboard bookmarkClipboard(&m_clipboardStore);
	auto clipboardItems = bookmarkClipboard.ReadBookmarks();
	ASSERT_EQ(clipboardItems.size(), 1u);

	ExpectClipboardBookmarkMatchesCopiedBookmark(clipboardItems[0].get(), &copiedBookmark);
}

TEST_F(BookmarkContextMenuSingleBookmarkTest, Paste)
{
	// This bookmark is explicitly copied before the menu is constructed. That's because the paste
	// menu item will be disabled if there are no bookmarks on the clipboard, which will then cause
	// the test to fail. Copying the bookmark first ensures that the paste item will be enabled when
	// the menu is built.
	auto *bookmark = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetOtherBookmarksFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"f:\\"), 0);
	CopiedBookmark copiedBookmark(bookmark);
	ASSERT_TRUE(
		BookmarkHelper::CopyBookmarkItems(&m_clipboardStore, &m_bookmarkTree, { bookmark }, false));

	MenuViewFake menuView;
	auto contextMenu = BuildContextMenu(&menuView);

	menuView.SelectItem(IDM_BOOKMARK_CONTEXT_MENU_PASTE, false, false);

	const auto *parentFolder = m_bookmark->GetParent();
	ASSERT_EQ(parentFolder->GetChildren().size(), 2u);

	const auto &pastedBookmark = parentFolder->GetChildren()[1];
	ExpectClipboardBookmarkMatchesCopiedBookmark(pastedBookmark.get(), &copiedBookmark);
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
