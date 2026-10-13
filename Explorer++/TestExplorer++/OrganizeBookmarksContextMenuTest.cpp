// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/UI/OrganizeBookmarksContextMenu.h"
#include "AcceleratorManager.h"
#include "Bookmarks/BookmarkClipboard.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkTree.h"
#include "Bookmarks/UI/OrganizeBookmarksContextMenuDelegate.h"
#include "CopiedBookmark.h"
#include "MainResource.h"
#include "MenuViewTestApi.h"
#include "MenuViewTestHelper.h"
#include "ResourceLoaderFake.h"
#include "RootMenuViewTestApi.h"
#include "SimulatedClipboardStore.h"
#include "TestMenuView.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace testing;

namespace
{

class OrganizeBookmarksContextMenuDelegateFake : public OrganizeBookmarksContextMenuDelegate
{
public:
	OrganizeBookmarksContextMenuDelegateFake(const RawBookmarkItems &selectedItems = {}) :
		m_selectedItems(selectedItems)
	{
		ON_CALL(*this, CanSelectAllItems).WillByDefault([this]() { return m_canSelectAllItems; });
		ON_CALL(*this, GetSelectedItems).WillByDefault([this]() { return m_selectedItems; });
		ON_CALL(*this, GetSelectedChildItems).WillByDefault([this]() { return m_selectedItems; });
	}

	MOCK_METHOD(bool, CanSelectAllItems, (), (const, override));
	MOCK_METHOD(void, SelectAllItems, (), (override));
	MOCK_METHOD(RawBookmarkItems, GetSelectedItems, (), (const, override));
	MOCK_METHOD(RawBookmarkItems, GetSelectedChildItems, (const BookmarkItem *targetFolder),
		(const, override));
	MOCK_METHOD(void, CreateBookmark, (BookmarkItem * parentFolder, size_t index), (override));
	MOCK_METHOD(void, CreateFolder, (BookmarkItem * parentFolder, size_t index), (override));

	void SetCanSelectAllItems(bool canSelectAllItems)
	{
		m_canSelectAllItems = canSelectAllItems;
	}

private:
	RawBookmarkItems m_selectedItems;
	bool m_canSelectAllItems = false;
};

}

class OrganizeBookmarksContextMenuTest : public Test
{
protected:
	OrganizeBookmarksContextMenuTest()
	{
		auto *parentFolder =
			m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
				std::make_unique<BookmarkItem>(std::nullopt, L"Parent folder", std::nullopt), 0);

		m_targetFolder = m_bookmarkTree.AddBookmarkItem(parentFolder,
			std::make_unique<BookmarkItem>(std::nullopt, L"Target folder", std::nullopt), 0);
		m_bookmarkTree.AddBookmarkItem(m_targetFolder,
			std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 1", L"c:\\"), 0);
		m_bookmarkTree.AddBookmarkItem(m_targetFolder,
			std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 2", L"d:\\"), 1);

		m_bookmarkToCopy = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetOtherBookmarksFolder(),
			std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 3", L"e:\\"), 0);
	}

	std::unique_ptr<OrganizeBookmarksContextMenu> BuildContextMenu(MenuView *menuView,
		OrganizeBookmarksContextMenuDelegate *delegate, BookmarkItem *targetFolder = nullptr)
	{
		return std::make_unique<OrganizeBookmarksContextMenu>(menuView, &m_acceleratorManager,
			&m_bookmarkTree,
			targetFolder ? targetFolder->GetWeakPtr() : m_targetFolder->GetWeakPtr(), delegate,
			&m_clipboardStore, &m_resourceLoader);
	}

	SimulatedClipboardStore m_clipboardStore;
	AcceleratorManager m_acceleratorManager;
	ResourceLoaderFake m_resourceLoader;

	BookmarkTree m_bookmarkTree;
	BookmarkItem *m_targetFolder = nullptr;
	BookmarkItem *m_bookmarkToCopy = nullptr;
};

TEST_F(OrganizeBookmarksContextMenuTest, ItemStatesWithNoSelection)
{
	TestMenuView menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate;
	auto menu = BuildContextMenu(&menuView, &delegate);

	EXPECT_FALSE(
		MenuViewTestApi::IsItemEnabled(&menuView, OrganizeBookmarksContextMenu::Item::Cut));
	EXPECT_FALSE(
		MenuViewTestApi::IsItemEnabled(&menuView, OrganizeBookmarksContextMenu::Item::Copy));
	EXPECT_FALSE(
		MenuViewTestApi::IsItemEnabled(&menuView, OrganizeBookmarksContextMenu::Item::Delete));
}

TEST_F(OrganizeBookmarksContextMenuTest, ItemStatesWithSelection)
{
	TestMenuView menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate({ m_targetFolder->GetChildren()[0].get() });
	auto menu = BuildContextMenu(&menuView, &delegate);

	EXPECT_TRUE(MenuViewTestApi::IsItemEnabled(&menuView, OrganizeBookmarksContextMenu::Item::Cut));
	EXPECT_TRUE(
		MenuViewTestApi::IsItemEnabled(&menuView, OrganizeBookmarksContextMenu::Item::Copy));
	EXPECT_TRUE(
		MenuViewTestApi::IsItemEnabled(&menuView, OrganizeBookmarksContextMenu::Item::Delete));
}

TEST_F(OrganizeBookmarksContextMenuTest, PasteStateWithEmptyClipboard)
{
	TestMenuView menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate;
	auto menu = BuildContextMenu(&menuView, &delegate);

	// There are no bookmarks on the clipboard, so it shouldn't be possible to paste.
	EXPECT_FALSE(
		MenuViewTestApi::IsItemEnabled(&menuView, OrganizeBookmarksContextMenu::Item::Paste));
}

TEST_F(OrganizeBookmarksContextMenuTest, PasteStateWithNonEmptyClipboard)
{
	BookmarkHelper::CopyBookmarkItems(&m_clipboardStore, &m_bookmarkTree, { m_bookmarkToCopy },
		ClipboardAction::Copy);

	TestMenuView menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate;
	auto menu = BuildContextMenu(&menuView, &delegate);

	EXPECT_TRUE(
		MenuViewTestApi::IsItemEnabled(&menuView, OrganizeBookmarksContextMenu::Item::Paste));
}

TEST_F(OrganizeBookmarksContextMenuTest, SelectAllDisabledState)
{
	TestMenuView menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate;
	delegate.SetCanSelectAllItems(false);
	auto menu = BuildContextMenu(&menuView, &delegate);

	EXPECT_FALSE(
		MenuViewTestApi::IsItemEnabled(&menuView, OrganizeBookmarksContextMenu::Item::SelectAll));
}

TEST_F(OrganizeBookmarksContextMenuTest, SelectAllEnabledStateWithNoChildItems)
{
	auto *emptyFolder = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetOtherBookmarksFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Empty folder", std::nullopt));

	TestMenuView menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate;
	delegate.SetCanSelectAllItems(true);
	auto menu = BuildContextMenu(&menuView, &delegate, emptyFolder);

	// Although the delegate allows all items to be selected, there are no child items in the target
	// folder, so there's nothing to select. Therefore, the select all menu item should remain
	// disabled.
	EXPECT_FALSE(
		MenuViewTestApi::IsItemEnabled(&menuView, OrganizeBookmarksContextMenu::Item::SelectAll));
}

TEST_F(OrganizeBookmarksContextMenuTest, SelectAllEnabledStateWithChildItems)
{
	TestMenuView menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate;
	delegate.SetCanSelectAllItems(true);
	auto menu = BuildContextMenu(&menuView, &delegate);

	EXPECT_TRUE(
		MenuViewTestApi::IsItemEnabled(&menuView, OrganizeBookmarksContextMenu::Item::SelectAll));
}

TEST_F(OrganizeBookmarksContextMenuTest, NewFolderWithNoSelection)
{
	TestMenuView menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate;
	auto menu = BuildContextMenu(&menuView, &delegate);

	// There are no selected items, so the new folder should be added to the last position in the
	// target folder.
	EXPECT_CALL(delegate, CreateFolder(m_targetFolder, 2));
	RootMenuViewTestApi::SelectItem(&menuView, OrganizeBookmarksContextMenu::Item::NewFolder, false,
		false);
}

TEST_F(OrganizeBookmarksContextMenuTest, NewFolderWithSelection)
{
	TestMenuView menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate({ m_targetFolder->GetChildren()[0].get() });
	auto menu = BuildContextMenu(&menuView, &delegate);

	// In this case, the first item in the target folder is selected. So, the new folder should be
	// added after that item.
	EXPECT_CALL(delegate, CreateFolder(m_targetFolder, 1));
	RootMenuViewTestApi::SelectItem(&menuView, OrganizeBookmarksContextMenu::Item::NewFolder, false,
		false);
}

TEST_F(OrganizeBookmarksContextMenuTest, Cut)
{
	TestMenuView menuView;

	auto *bookmark = m_targetFolder->GetChildren()[0].get();
	CopiedBookmark copiedBookmark(*bookmark);
	OrganizeBookmarksContextMenuDelegateFake delegate({ bookmark });

	auto menu = BuildContextMenu(&menuView, &delegate);

	RootMenuViewTestApi::SelectItem(&menuView, OrganizeBookmarksContextMenu::Item::Cut, false,
		false);
	EXPECT_EQ(m_targetFolder->GetChildren().size(), 1u);

	BookmarkClipboard bookmarkClipboard(&m_clipboardStore);
	auto clipboardItems = bookmarkClipboard.ReadBookmarks();
	EXPECT_THAT(clipboardItems, ElementsAre(Pointee(copiedBookmark)));
}

TEST_F(OrganizeBookmarksContextMenuTest, Copy)
{
	TestMenuView menuView;

	auto *bookmark = m_targetFolder->GetChildren()[0].get();
	CopiedBookmark copiedBookmark(*bookmark);
	OrganizeBookmarksContextMenuDelegateFake delegate({ bookmark });

	auto menu = BuildContextMenu(&menuView, &delegate);

	RootMenuViewTestApi::SelectItem(&menuView, OrganizeBookmarksContextMenu::Item::Copy, false,
		false);

	BookmarkClipboard bookmarkClipboard(&m_clipboardStore);
	auto clipboardItems = bookmarkClipboard.ReadBookmarks();
	EXPECT_THAT(clipboardItems, ElementsAre(Pointee(copiedBookmark)));
}

TEST_F(OrganizeBookmarksContextMenuTest, PasteWithNoSelection)
{
	CopiedBookmark copiedBookmark(*m_bookmarkToCopy);
	BookmarkHelper::CopyBookmarkItems(&m_clipboardStore, &m_bookmarkTree, { m_bookmarkToCopy },
		ClipboardAction::Copy);

	TestMenuView menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate;
	auto menu = BuildContextMenu(&menuView, &delegate);

	RootMenuViewTestApi::SelectItem(&menuView, OrganizeBookmarksContextMenu::Item::Paste, false,
		false);
	ASSERT_EQ(m_targetFolder->GetChildren().size(), 3u);
	EXPECT_THAT(m_targetFolder->GetChildren()[2], Pointee(copiedBookmark));
}

TEST_F(OrganizeBookmarksContextMenuTest, PasteWithSelection)
{
	CopiedBookmark copiedBookmark(*m_bookmarkToCopy);
	BookmarkHelper::CopyBookmarkItems(&m_clipboardStore, &m_bookmarkTree, { m_bookmarkToCopy },
		ClipboardAction::Copy);

	TestMenuView menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate({ m_targetFolder->GetChildren()[0].get() });
	auto menu = BuildContextMenu(&menuView, &delegate);

	RootMenuViewTestApi::SelectItem(&menuView, OrganizeBookmarksContextMenu::Item::Paste, false,
		false);
	ASSERT_EQ(m_targetFolder->GetChildren().size(), 3u);
	EXPECT_THAT(m_targetFolder->GetChildren()[1], Pointee(copiedBookmark));
}

TEST_F(OrganizeBookmarksContextMenuTest, SelectAll)
{
	TestMenuView menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate;
	delegate.SetCanSelectAllItems(true);
	auto menu = BuildContextMenu(&menuView, &delegate);

	EXPECT_CALL(delegate, SelectAllItems());
	RootMenuViewTestApi::SelectItem(&menuView, OrganizeBookmarksContextMenu::Item::SelectAll, false,
		false);
}

TEST_F(OrganizeBookmarksContextMenuTest, SelectionAfterTargetFolderDestroyed)
{
	TestMenuView menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate;
	auto menu = BuildContextMenu(&menuView, &delegate);

	m_bookmarkTree.RemoveBookmarkItem(m_targetFolder);

	// The target folder was removed, so this call should have no effect, but should still be safe.
	EXPECT_CALL(delegate, CreateFolder).Times(0);
	RootMenuViewTestApi::SelectItem(&menuView, OrganizeBookmarksContextMenu::Item::NewFolder, false,
		false);
}

TEST_F(OrganizeBookmarksContextMenuTest, SelectionAfterParentFolderDestroyed)
{
	TestMenuView menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate;
	auto menu = BuildContextMenu(&menuView, &delegate);

	m_bookmarkTree.RemoveBookmarkItem(m_targetFolder->GetParent());

	// In this case, the target folder was implicitly removed (by removing the parent folder), but
	// this call should again be safe.
	EXPECT_CALL(delegate, CreateFolder).Times(0);
	RootMenuViewTestApi::SelectItem(&menuView, OrganizeBookmarksContextMenu::Item::NewFolder, false,
		false);
}
