// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/UI/BookmarksToolbar.h"
#include "AcceleratorManager.h"
#include "Bookmarks/BookmarkTree.h"
#include "Bookmarks/UI/Views/BookmarksToolbarView.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "Config.h"
#include "IconFetcher.h"
#include "ResourceLoaderFake.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellTestHelper.h"
#include "SimulatedClipboardStore.h"
#include <boost/range/combine.hpp>
#include <gtest/gtest.h>

namespace
{

class IconFetcherFake : public IconFetcher
{
public:
	void QueueIconTask(std::wstring_view path, Callback callback) override
	{
		UNREFERENCED_PARAMETER(path);
		UNREFERENCED_PARAMETER(callback);
	}

	void QueueIconTask(PCIDLIST_ABSOLUTE pidl, Callback callback) override
	{
		UNREFERENCED_PARAMETER(pidl);
		UNREFERENCED_PARAMETER(callback);
	}

	void ClearQueue() override
	{
	}

	int GetCachedIconIndexOrDefault(const std::wstring &itemPath,
		DefaultIconType defaultIconType) const override
	{
		UNREFERENCED_PARAMETER(itemPath);
		UNREFERENCED_PARAMETER(defaultIconType);

		return 0;
	}

	std::optional<int> GetCachedIconIndex(const std::wstring &itemPath) const override
	{
		UNREFERENCED_PARAMETER(itemPath);

		return std::nullopt;
	}
};

}

class BookmarksToolbarTest : public BrowserTestBase
{
protected:
	BookmarksToolbarTest() :
		m_bookmarkTree(CreateBookmarkTree()),
		m_browser(AddBrowser()),
		m_bookmarksToolbarView(BookmarksToolbarView::Create(m_browser->GetHWND(), &m_config)),
		m_bookmarksToolbar(
			BookmarksToolbar::Create(m_bookmarksToolbarView, m_browser, &m_acceleratorManager,
				&m_resourceLoader, &m_iconFetcher, m_bookmarkTree.get(), &m_clipboardStore))
	{
	}

	static std::unique_ptr<BookmarkTree> CreateBookmarkTree()
	{
		auto bookmarkTree = std::make_unique<BookmarkTree>();
		bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksToolbarFolder(),
			std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt));
		bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksToolbarFolder(),
			std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"c:\\path\\to\\folder"));
		bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksToolbarFolder(),
			std::make_unique<BookmarkItem>(std::nullopt, L"Folder 2", std::nullopt));
		return bookmarkTree;
	}

	void NavigateTab(Tab *tab, const std::wstring &path)
	{
		auto pidl = CreateSimplePidlForTest(path);
		auto navigateParams = NavigateParams::Normal(pidl.Raw());
		tab->GetShellBrowser()->GetNavigationController()->Navigate(navigateParams);
	}

	void VerifyToolbarButtons()
	{
		const auto &buttons = m_bookmarksToolbarView->GetButtons();
		const auto &bookmarks = m_bookmarkTree->GetBookmarksToolbarFolder()->GetChildren();
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

	Config m_config;
	AcceleratorManager m_acceleratorManager;
	ResourceLoaderFake m_resourceLoader;
	IconFetcherFake m_iconFetcher;
	SimulatedClipboardStore m_clipboardStore;

	std::unique_ptr<BookmarkTree> m_bookmarkTree;

	BrowserWindowFake *const m_browser;
	BookmarksToolbarView *const m_bookmarksToolbarView;
	BookmarksToolbar *const m_bookmarksToolbar;
};

TEST_F(BookmarksToolbarTest, InitialBookmarks)
{
	VerifyToolbarButtons();
}

TEST_F(BookmarksToolbarTest, AddBookmarks)
{
	m_bookmarkTree->AddBookmarkItem(m_bookmarkTree->GetBookmarksToolbarFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 3", std::nullopt));
	m_bookmarkTree->AddBookmarkItem(m_bookmarkTree->GetBookmarksToolbarFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Windows", L"c:\\windows"));

	// These items are being created in a different folder, so shouldn't be added to the toolbar.
	m_bookmarkTree->AddBookmarkItem(m_bookmarkTree->GetOtherBookmarksFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder", std::nullopt));
	m_bookmarkTree->AddBookmarkItem(m_bookmarkTree->GetOtherBookmarksFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"d:\\"));

	VerifyToolbarButtons();
}

TEST_F(BookmarksToolbarTest, UpdateBookmarks)
{
	const auto &bookmarks = m_bookmarkTree->GetBookmarksToolbarFolder()->GetChildren();
	bookmarks[0]->SetName(L"Updated folder name");
	bookmarks[1]->SetName(L"Updated bookmark name");

	VerifyToolbarButtons();
}

TEST_F(BookmarksToolbarTest, MoveBookmarks)
{
	auto *bookmarksToolbarFolder = m_bookmarkTree->GetBookmarksToolbarFolder();
	m_bookmarkTree->MoveBookmarkItem(bookmarksToolbarFolder->GetChildren()[0].get(),
		bookmarksToolbarFolder, 2);

	m_bookmarkTree->MoveBookmarkItem(bookmarksToolbarFolder->GetChildren()[1].get(),
		m_bookmarkTree->GetOtherBookmarksFolder(), 0);

	VerifyToolbarButtons();
}

TEST_F(BookmarksToolbarTest, RemoveBookmarks)
{
	std::vector<BookmarkItem *> childBookmarks;

	for (const auto &bookmark : m_bookmarkTree->GetBookmarksToolbarFolder()->GetChildren())
	{
		childBookmarks.push_back(bookmark.get());
	}

	for (auto *childBookmark : childBookmarks)
	{
		m_bookmarkTree->RemoveBookmarkItem(childBookmark);
	}

	VerifyToolbarButtons();
}

TEST_F(BookmarksToolbarTest, OpenBookmarkOnClick)
{
	auto *tab = m_browser->AddTab();
	m_browser->ActivateTabAtIndex(0);

	NavigateTab(tab, L"c:\\original\\path");

	const auto &buttons = m_bookmarksToolbarView->GetButtons();
	const auto &bookmarks = m_bookmarkTree->GetBookmarksToolbarFolder()->GetChildren();
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
