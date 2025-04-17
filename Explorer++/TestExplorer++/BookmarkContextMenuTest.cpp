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
#include "PopupMenuView.h"
#include "ResourceLoaderFake.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellTestHelper.h"
#include "../Helper/Clipboard.h"
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

	BookmarkContextMenuTestBase() : m_browser(AddBrowser()), m_tab(m_browser->AddTab())
	{
		m_browser->ActivateTabAtIndex(0);

		NavigateTab(m_tab, L"c:\\original\\path");
	}

	void NavigateTab(Tab *tab, const std::wstring &path)
	{
		auto pidl = CreateSimplePidlForTest(path);
		auto navigateParams = NavigateParams::Normal(pidl.Raw());
		tab->GetShellBrowser()->GetNavigationController()->Navigate(navigateParams);
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

	BookmarkTree m_bookmarkTree;

	BrowserWindowFake *const m_browser;
	Tab *const m_tab;

	PopupMenuView m_popupMenu;
};

class BookmarkContextMenuSingleBookmarkTest : public BookmarkContextMenuTestBase
{
protected:
	BookmarkContextMenuSingleBookmarkTest() :
		m_bookmark(m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
			std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"c:\\bookmarked\\folder"),
			0)),
		m_contextMenu(&m_popupMenu, &m_acceleratorManager, &m_bookmarkTree, { m_bookmark },
			&m_resourceLoader, m_browser, m_browser->GetHWND())
	{
	}

	BookmarkItem *const m_bookmark;
	BookmarkContextMenu m_contextMenu;
};

TEST_F(BookmarkContextMenuSingleBookmarkTest, Open)
{
	m_popupMenu.SelectItem(IDM_BOOKMARK_CONTEXT_MENU_OPEN, false, false);

	EXPECT_EQ(m_tab->GetShellBrowser()->GetNavigationController()->GetNumHistoryEntries(), 2);

	const auto *currentEntry =
		m_tab->GetShellBrowser()->GetNavigationController()->GetCurrentEntry();
	ASSERT_NE(currentEntry, nullptr);
	EXPECT_EQ(currentEntry->GetPidl(), CreateSimplePidlForTest(m_bookmark->GetLocation()));
}

TEST_F(BookmarkContextMenuSingleBookmarkTest, Cut)
{
	Clipboard clipboard;
	clipboard.Clear();

	CopiedBookmark copiedBookmark(m_bookmark);
	const auto *parentFolder = m_bookmark->GetParent();

	m_popupMenu.SelectItem(IDM_BOOKMARK_CONTEXT_MENU_CUT, false, false);

	// Cutting the bookmark should have removed it from the tree.
	ASSERT_TRUE(parentFolder->GetChildren().empty());

	BookmarkClipboard bookmarkClipboard;
	auto clipboardItems = bookmarkClipboard.ReadBookmarks();
	ASSERT_EQ(clipboardItems.size(), 1u);

	ExpectClipboardBookmarkMatchesCopiedBookmark(clipboardItems[0].get(), &copiedBookmark);
}

TEST_F(BookmarkContextMenuSingleBookmarkTest, Copy)
{
	Clipboard clipboard;
	clipboard.Clear();

	CopiedBookmark copiedBookmark(m_bookmark);

	m_popupMenu.SelectItem(IDM_BOOKMARK_CONTEXT_MENU_COPY, false, false);

	BookmarkClipboard bookmarkClipboard;
	auto clipboardItems = bookmarkClipboard.ReadBookmarks();
	ASSERT_EQ(clipboardItems.size(), 1u);

	ExpectClipboardBookmarkMatchesCopiedBookmark(clipboardItems[0].get(), &copiedBookmark);
}

TEST_F(BookmarkContextMenuSingleBookmarkTest, Paste)
{
	Clipboard clipboard;
	clipboard.Clear();

	auto *bookmark = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetOtherBookmarksFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"f:\\"), 0);
	CopiedBookmark copiedBookmark(bookmark);
	ASSERT_TRUE(BookmarkHelper::CopyBookmarkItems(&m_bookmarkTree, { bookmark }, false));

	m_popupMenu.SelectItem(IDM_BOOKMARK_CONTEXT_MENU_PASTE, false, false);

	const auto *parentFolder = m_bookmark->GetParent();
	ASSERT_EQ(parentFolder->GetChildren().size(), 2u);

	const auto &pastedBookmark = parentFolder->GetChildren()[1];
	ExpectClipboardBookmarkMatchesCopiedBookmark(pastedBookmark.get(), &copiedBookmark);
}

TEST_F(BookmarkContextMenuSingleBookmarkTest, Delete)
{
	MockFunction<void(const std::wstring &guid)> removedCallback;
	m_bookmarkTree.bookmarkItemRemovedSignal.AddObserver(removedCallback.AsStdFunction());

	EXPECT_CALL(removedCallback, Call(m_bookmark->GetGUID()));
	m_popupMenu.SelectItem(IDM_BOOKMARK_CONTEXT_MENU_DELETE, false, false);
}
