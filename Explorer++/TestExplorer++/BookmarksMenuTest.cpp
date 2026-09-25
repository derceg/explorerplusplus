// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/UI/BookmarksMenu.h"
#include "Bookmarks/BookmarkIconManager.h"
#include "Bookmarks/BookmarkTree.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "IconFetcherFake.h"
#include "MenuTestHost.h"
#include "MenuViewTestHelper.h"
#include "PidlTestHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "../Helper/DragDropHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class BookmarksMenuTest : public BrowserTestBase
{
protected:
	BookmarksMenuTest() :
		m_browser(AddBrowser()),
		m_tab(m_browser->AddTab(L"c:\\initial\\folder")),
		m_iconManager(&m_resourceLoader, &m_iconFetcher, 16, 16)
	{
	}

	[[nodiscard]] BookmarksMenu BuildMenu(MenuView *menuView, BookmarkItem *targetFolder,
		BookmarkMenuBuilder::IncludePredicate includePredicate = {})
	{
		return { menuView, &m_acceleratorManager, &m_bookmarkTree, targetFolder, &m_iconManager,
			m_browser, m_browser->GetHWND(), &m_platformContext, &m_resourceLoader,
			includePredicate };
	}

	void VerifyMenuItems(const MenuView *menuView, const BookmarkItem *bookmarkFolder,
		BookmarkMenuBuilder::IncludePredicate includePredicate = {})
	{
		if (bookmarkFolder->GetChildren().empty())
		{
			// The folder has no children, but the menu should still have a single item added
			// (indicating that the menu is empty).
			ASSERT_EQ(menuView->GetNumItems(), 1);
			MenuViewTestHelper::ExpectItemEnabled(menuView, menuView->GetItemIdForTesting(0),
				false);
			return;
		}

		std::vector<const BookmarkItem *> includedChildren;

		for (const auto &child : bookmarkFolder->GetChildren())
		{
			if (includePredicate && !includePredicate(child.get()))
			{
				continue;
			}

			includedChildren.push_back(child.get());
		}

		ASSERT_EQ(menuView->GetNumItems(), static_cast<int>(includedChildren.size()));

		for (int i = 0; i < menuView->GetNumItems(); i++)
		{
			const auto *child = includedChildren[i];

			UINT id = menuView->GetItemIdForTesting(i);
			EXPECT_EQ(menuView->GetItemTextForTesting(id), child->GetName());

			if (child->IsBookmark())
			{
				EXPECT_EQ(menuView->GetItemHelpText(id), child->GetLocation());
			}
			else
			{
				VerifyMenuItems(menuView->GetSubMenuViewForTesting(id), child);
			}
		}
	}

	void SimulateDropOnMenuView(MenuView *menuView, const MenuDropLocation &dropLocation,
		const std::vector<PidlAbsolute> &items)
	{
		auto *delegate = menuView->MaybeGetDelegateForItemForTesting(dropLocation.id);
		ASSERT_NE(delegate, nullptr);

		auto dropTarget = delegate->MaybeGetDropTargetForLocation(dropLocation);
		ASSERT_NE(dropTarget, nullptr);

		wil::com_ptr_nothrow<IDataObject> dataObject;
		HRESULT hr = CreateDataObjectForShellTransfer(items, &dataObject);
		ASSERT_HRESULT_SUCCEEDED(hr);

		DWORD effect = DROPEFFECT_COPY;
		hr = dropTarget->DragEnter(dataObject.get(), MK_LBUTTON, { 0, 0 }, &effect);
		ASSERT_HRESULT_SUCCEEDED(hr);

		hr = dropTarget->Drop(dataObject.get(), MK_LBUTTON, { 0, 0 }, &effect);
		ASSERT_HRESULT_SUCCEEDED(hr);
	}

	BrowserWindowFake *const m_browser;
	Tab *const m_tab;

	IconFetcherFake m_iconFetcher;
	BookmarkIconManager m_iconManager;
};

TEST_F(BookmarksMenuTest, Items)
{
	auto *targetFolder = m_bookmarkTree.GetOtherBookmarksFolder();
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 1", L"c:\\"));

	auto *subFolder1 = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(subFolder1,
		std::make_unique<BookmarkItem>(std::nullopt, L"Nested bookmark 1", L"c:\\nested"));

	auto *subFolder2 = m_bookmarkTree.AddBookmarkItem(subFolder1,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 2", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(subFolder2,
		std::make_unique<BookmarkItem>(std::nullopt, L"Nested bookmark 2", L"c:\\nested 2"));

	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 2", L"e:\\"));

	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Empty folder", std::nullopt));

	MenuTestHost menuHost;
	auto *menuView = menuHost.GetView();
	auto menu = BuildMenu(menuView, targetFolder);

	VerifyMenuItems(menuView, targetFolder);
}

TEST_F(BookmarksMenuTest, ItemsWithIncludePredicate)
{
	auto *targetFolder = m_bookmarkTree.GetOtherBookmarksFolder();
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 1", L"c:\\"));

	auto *subFolder1 = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(subFolder1,
		std::make_unique<BookmarkItem>(std::nullopt, L"Nested bookmark 1", L"c:\\nested"));

	auto *subFolder2 = m_bookmarkTree.AddBookmarkItem(subFolder1,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder 2", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(subFolder2,
		std::make_unique<BookmarkItem>(std::nullopt, L"Nested bookmark 2", L"c:\\nested 2"));

	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 2", L"e:\\"));

	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Empty folder", std::nullopt));

	auto includePredicate = [](const BookmarkItem *bookmarkItem)
	{
		return bookmarkItem->IsBookmark();
	};

	MenuTestHost menuHost;
	auto *menuView = menuHost.GetView();
	auto menu = BuildMenu(menuView, targetFolder, includePredicate);

	VerifyMenuItems(menuView, targetFolder, includePredicate);
}

TEST_F(BookmarksMenuTest, Selection)
{
	auto *targetFolder = m_bookmarkTree.GetOtherBookmarksFolder();
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 1", L"c:\\"));
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 2", L"d:\\"));
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 3", L"e:\\"));

	MenuTestHost menuHost;
	auto *menuView = menuHost.GetView();
	auto menu = BuildMenu(menuView, targetFolder);

	ASSERT_EQ(menuView->GetNumItems(), static_cast<int>(targetFolder->GetChildren().size()));

	for (int i = 0; i < menuView->GetNumItems(); i++)
	{
		menuHost.SelectItemAtIndex(i, false, false);
		EXPECT_THAT(m_tab->GetShellBrowser()->GetDirectoryPath(),
			StrCaseEq(targetFolder->GetChildAtIndex(i)->GetLocation()));
	}
}

TEST_F(BookmarksMenuTest, MiddleClickBookmark)
{
	auto *targetFolder = m_bookmarkTree.GetOtherBookmarksFolder();
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"c:\\"));

	MenuTestHost menuHost;
	auto *menuView = menuHost.GetView();
	auto menu = BuildMenu(menuView, targetFolder);

	menuHost.MiddleClickItemAtIndex(0, false, false);

	auto *tabContainer = m_browser->GetActiveTabContainer();
	ASSERT_EQ(tabContainer->GetNumTabs(), 2);
	EXPECT_THAT(tabContainer->GetTabByIndex(1).GetShellBrowser()->GetDirectoryPath(),
		StrCaseEq(targetFolder->GetChildAtIndex(0)->GetLocation()));
}

TEST_F(BookmarksMenuTest, MiddleClickFolder)
{
	auto *targetFolder = m_bookmarkTree.GetOtherBookmarksFolder();
	auto *subFolder = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(subFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 1", L"c:\\"));
	m_bookmarkTree.AddBookmarkItem(subFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 2", L"d:\\"));
	m_bookmarkTree.AddBookmarkItem(subFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 3", L"e:\\"));

	MenuTestHost menuHost;
	auto *menuView = menuHost.GetView();
	auto menu = BuildMenu(menuView, targetFolder);

	menuHost.MiddleClickItemAtIndex(0, false, false);

	// Each of the bookmarks in the folder should have been opened in a new tab.
	auto *tabContainer = m_browser->GetActiveTabContainer();
	int numChildren = static_cast<int>(subFolder->GetChildren().size());
	ASSERT_EQ(tabContainer->GetNumTabs(), numChildren + 1);

	for (int i = 0; i < numChildren; i++)
	{
		EXPECT_THAT(tabContainer->GetTabByIndex(i + 1).GetShellBrowser()->GetDirectoryPath(),
			StrCaseEq(subFolder->GetChildAtIndex(i)->GetLocation()));
	}
}

TEST_F(BookmarksMenuTest, DropAtIndex)
{
	auto *targetFolder = m_bookmarkTree.GetOtherBookmarksFolder();
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 1", L"c:\\"));
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 2", L"d:\\"));
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 3", L"e:\\"));

	MenuTestHost menuHost;
	auto *menuView = menuHost.GetView();
	auto menu = BuildMenu(menuView, targetFolder);

	PidlAbsolute dropItem = CreateSimplePidlForTest(L"c:\\dropped-folder");
	SimulateDropOnMenuView(menuView,
		{ menuView->GetItemIdForTesting(1), MenuDropLocation::Position::Before }, { dropItem });

	ASSERT_EQ(targetFolder->GetChildren().size(), 4u);

	auto *droppedBookmark = targetFolder->GetChildAtIndex(1);
	ASSERT_TRUE(droppedBookmark->IsBookmark());
	EXPECT_EQ(droppedBookmark->GetName(), L"dropped-folder");
	EXPECT_THAT(droppedBookmark->GetLocation(), StrCaseEq(L"c:\\dropped-folder"));
}

TEST_F(BookmarksMenuTest, DropOnFolder)
{
	auto *targetFolder = m_bookmarkTree.GetOtherBookmarksFolder();
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"c:\\"));
	auto *subFolder = m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Folder", std::nullopt));
	m_bookmarkTree.AddBookmarkItem(subFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Nested bookmark", L"d:\\"));

	MenuTestHost menuHost;
	auto *menuView = menuHost.GetView();
	auto menu = BuildMenu(menuView, targetFolder);

	PidlAbsolute dropItem = CreateSimplePidlForTest(L"c:\\dropped-folder");
	SimulateDropOnMenuView(menuView,
		{ menuView->GetItemIdForTesting(1), MenuDropLocation::Position::On }, { dropItem });

	ASSERT_EQ(subFolder->GetChildren().size(), 2u);

	// The dropped bookmark should be created after the existing bookmark in the subfolder.
	auto *droppedBookmark = subFolder->GetChildAtIndex(1);
	ASSERT_TRUE(droppedBookmark->IsBookmark());
	EXPECT_EQ(droppedBookmark->GetName(), L"dropped-folder");
	EXPECT_THAT(droppedBookmark->GetLocation(), StrCaseEq(L"c:\\dropped-folder"));
}

TEST_F(BookmarksMenuTest, DropOnEmptyItem)
{
	auto *targetFolder = m_bookmarkTree.GetOtherBookmarksFolder();

	MenuTestHost menuHost;
	auto *menuView = menuHost.GetView();
	auto menu = BuildMenu(menuView, targetFolder);

	// The target folder is empty, so only the "empty" menu item should be shown. This checks that
	// dropping on that item results in the dropped bookmark being added to the parent.
	PidlAbsolute dropItem = CreateSimplePidlForTest(L"c:\\dropped-folder");
	SimulateDropOnMenuView(menuView,
		{ menuView->GetItemIdForTesting(0), MenuDropLocation::Position::On }, { dropItem });

	ASSERT_EQ(targetFolder->GetChildren().size(), 1u);

	auto *droppedBookmark = targetFolder->GetChildAtIndex(0);
	ASSERT_TRUE(droppedBookmark->IsBookmark());
	EXPECT_EQ(droppedBookmark->GetName(), L"dropped-folder");
	EXPECT_THAT(droppedBookmark->GetLocation(), StrCaseEq(L"c:\\dropped-folder"));
}

TEST_F(BookmarksMenuTest, DropAfterEmptyItem)
{
	auto *targetFolder = m_bookmarkTree.GetOtherBookmarksFolder();

	MenuTestHost menuHost;
	auto *menuView = menuHost.GetView();
	auto menu = BuildMenu(menuView, targetFolder);

	PidlAbsolute dropItem = CreateSimplePidlForTest(L"c:\\dropped-folder");
	SimulateDropOnMenuView(menuView,
		{ menuView->GetItemIdForTesting(0), MenuDropLocation::Position::After }, { dropItem });

	ASSERT_EQ(targetFolder->GetChildren().size(), 1u);

	auto *droppedBookmark = targetFolder->GetChildAtIndex(0);
	ASSERT_TRUE(droppedBookmark->IsBookmark());
	EXPECT_EQ(droppedBookmark->GetName(), L"dropped-folder");
	EXPECT_THAT(droppedBookmark->GetLocation(), StrCaseEq(L"c:\\dropped-folder"));
}

TEST_F(BookmarksMenuTest, ActionsAfterRemoval)
{
	auto *targetFolder = m_bookmarkTree.GetOtherBookmarksFolder();
	m_bookmarkTree.AddBookmarkItem(targetFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark", L"c:\\"));

	MenuTestHost menuHost;
	auto *menuView = menuHost.GetView();
	auto menu = BuildMenu(menuView, targetFolder);

	m_bookmarkTree.RemoveBookmarkItem(targetFolder->GetChildAtIndex(0));

	// Selecting an item that has been removed should have no effect.
	menuHost.SelectItemAtIndex(0, false, false);
	auto *tabContainer = m_browser->GetActiveTabContainer();
	EXPECT_THAT(m_tab->GetShellBrowser()->GetDirectoryPath(), StrCaseEq(L"c:\\initial\\folder"));

	menuHost.MiddleClickItemAtIndex(0, false, false);
	EXPECT_EQ(tabContainer->GetNumTabs(), 1);

	auto *delegate = menuView->MaybeGetDelegateForItemForTesting(menuView->GetItemIdForTesting(0));
	ASSERT_NE(delegate, nullptr);

	auto dropTarget = delegate->MaybeGetDropTargetForLocation(
		{ menuView->GetItemIdForTesting(0), MenuDropLocation::Position::On });
	EXPECT_EQ(dropTarget, nullptr);
}
