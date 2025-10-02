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
#include "MenuViewFake.h"
#include "ResourceLoaderFake.h"
#include "SimulatedClipboardStore.h"
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
	MOCK_METHOD(void, CreateFolder, (size_t index), (override));
	MOCK_METHOD(RawBookmarkItems, GetSelectedItems, (), (const, override));
	MOCK_METHOD(RawBookmarkItems, GetSelectedChildItems, (const BookmarkItem *targetFolder),
		(const, override));
	MOCK_METHOD(void, SelectOnly, (const BookmarkItem *bookmarkItem), (override));

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
			nullptr, &m_bookmarkTree, targetFolder ? targetFolder : m_targetFolder, delegate,
			&m_clipboardStore, &m_resourceLoader);
	}

	AcceleratorManager m_acceleratorManager;
	SimulatedClipboardStore m_clipboardStore;
	ResourceLoaderFake m_resourceLoader;

	BookmarkTree m_bookmarkTree;
	BookmarkItem *m_targetFolder = nullptr;
	BookmarkItem *m_bookmarkToCopy = nullptr;
};

TEST_F(OrganizeBookmarksContextMenuTest, ItemStatesWithNoSelection)
{
	MenuViewFake menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate;
	auto menu = BuildContextMenu(&menuView, &delegate);

	EXPECT_FALSE(menuView.IsItemEnabled(IDM_ORGANIZE_BOOKMARKS_CXMENU_CUT));
	EXPECT_FALSE(menuView.IsItemEnabled(IDM_ORGANIZE_BOOKMARKS_CXMENU_COPY));
	EXPECT_FALSE(menuView.IsItemEnabled(IDM_ORGANIZE_BOOKMARKS_CXMENU_DELETE));
}

TEST_F(OrganizeBookmarksContextMenuTest, ItemStatesWithSelection)
{
	MenuViewFake menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate({ m_targetFolder->GetChildren()[0].get() });
	auto menu = BuildContextMenu(&menuView, &delegate);

	EXPECT_TRUE(menuView.IsItemEnabled(IDM_ORGANIZE_BOOKMARKS_CXMENU_CUT));
	EXPECT_TRUE(menuView.IsItemEnabled(IDM_ORGANIZE_BOOKMARKS_CXMENU_COPY));
	EXPECT_TRUE(menuView.IsItemEnabled(IDM_ORGANIZE_BOOKMARKS_CXMENU_DELETE));
}

TEST_F(OrganizeBookmarksContextMenuTest, PasteStateWithEmptyClipboard)
{
	MenuViewFake menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate;
	auto menu = BuildContextMenu(&menuView, &delegate);

	// There are no bookmarks on the clipboard, so it shouldn't be possible to paste.
	EXPECT_FALSE(menuView.IsItemEnabled(IDM_ORGANIZE_BOOKMARKS_CXMENU_PASTE));
}

TEST_F(OrganizeBookmarksContextMenuTest, PasteStateWithNonEmptyClipboard)
{
	BookmarkHelper::CopyBookmarkItems(&m_clipboardStore, &m_bookmarkTree, { m_bookmarkToCopy },
		ClipboardAction::Copy);

	MenuViewFake menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate;
	auto menu = BuildContextMenu(&menuView, &delegate);

	EXPECT_TRUE(menuView.IsItemEnabled(IDM_ORGANIZE_BOOKMARKS_CXMENU_PASTE));
}

TEST_F(OrganizeBookmarksContextMenuTest, SelectAllDisabledState)
{
	MenuViewFake menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate;
	delegate.SetCanSelectAllItems(false);
	auto menu = BuildContextMenu(&menuView, &delegate);

	EXPECT_FALSE(menuView.IsItemEnabled(IDM_ORGANIZE_BOOKMARKS_CXMENU_SELECT_ALL));
}

TEST_F(OrganizeBookmarksContextMenuTest, SelectAllEnabledStateWithNoChildItems)
{
	auto *emptyFolder = m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetOtherBookmarksFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Empty folder", std::nullopt));

	MenuViewFake menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate;
	delegate.SetCanSelectAllItems(true);
	auto menu = BuildContextMenu(&menuView, &delegate, emptyFolder);

	// Although the delegate allows all items to be selected, there are no child items in the target
	// folder, so there's nothing to select. Therefore, the select all menu item should remain
	// disabled.
	EXPECT_FALSE(menuView.IsItemEnabled(IDM_ORGANIZE_BOOKMARKS_CXMENU_SELECT_ALL));
}

TEST_F(OrganizeBookmarksContextMenuTest, SelectAllEnabledStateWithChildItems)
{
	MenuViewFake menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate;
	delegate.SetCanSelectAllItems(true);
	auto menu = BuildContextMenu(&menuView, &delegate);

	EXPECT_TRUE(menuView.IsItemEnabled(IDM_ORGANIZE_BOOKMARKS_CXMENU_SELECT_ALL));
}

TEST_F(OrganizeBookmarksContextMenuTest, NewFolderWithNoSelection)
{
	MenuViewFake menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate;
	auto menu = BuildContextMenu(&menuView, &delegate);

	// There are no selected items, so the new folder should be added to the last position in the
	// target folder.
	EXPECT_CALL(delegate, CreateFolder(2));
	menuView.SelectItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_NEW_FOLDER, false, false);
}

TEST_F(OrganizeBookmarksContextMenuTest, NewFolderWithSelection)
{
	MenuViewFake menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate({ m_targetFolder->GetChildren()[0].get() });
	auto menu = BuildContextMenu(&menuView, &delegate);

	// In this case, the first item in the target folder is selected. So, the new folder should be
	// added after that item.
	EXPECT_CALL(delegate, CreateFolder(1));
	menuView.SelectItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_NEW_FOLDER, false, false);
}

TEST_F(OrganizeBookmarksContextMenuTest, Cut)
{
	MenuViewFake menuView;

	auto *bookmark = m_targetFolder->GetChildren()[0].get();
	CopiedBookmark copiedBookmark(*bookmark);
	OrganizeBookmarksContextMenuDelegateFake delegate({ bookmark });

	auto menu = BuildContextMenu(&menuView, &delegate);

	menuView.SelectItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_CUT, false, false);
	EXPECT_EQ(m_targetFolder->GetChildren().size(), 1u);

	BookmarkClipboard bookmarkClipboard(&m_clipboardStore);
	auto clipboardItems = bookmarkClipboard.ReadBookmarks();
	EXPECT_THAT(clipboardItems, ElementsAre(Pointee(copiedBookmark)));
}

TEST_F(OrganizeBookmarksContextMenuTest, Copy)
{
	MenuViewFake menuView;

	auto *bookmark = m_targetFolder->GetChildren()[0].get();
	CopiedBookmark copiedBookmark(*bookmark);
	OrganizeBookmarksContextMenuDelegateFake delegate({ bookmark });

	auto menu = BuildContextMenu(&menuView, &delegate);

	menuView.SelectItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_COPY, false, false);

	BookmarkClipboard bookmarkClipboard(&m_clipboardStore);
	auto clipboardItems = bookmarkClipboard.ReadBookmarks();
	EXPECT_THAT(clipboardItems, ElementsAre(Pointee(copiedBookmark)));
}

TEST_F(OrganizeBookmarksContextMenuTest, PasteWithNoSelection)
{
	CopiedBookmark copiedBookmark(*m_bookmarkToCopy);
	BookmarkHelper::CopyBookmarkItems(&m_clipboardStore, &m_bookmarkTree, { m_bookmarkToCopy },
		ClipboardAction::Copy);

	MenuViewFake menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate;
	auto menu = BuildContextMenu(&menuView, &delegate);

	menuView.SelectItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_PASTE, false, false);
	ASSERT_EQ(m_targetFolder->GetChildren().size(), 3u);
	EXPECT_THAT(m_targetFolder->GetChildren()[2], Pointee(copiedBookmark));
}

TEST_F(OrganizeBookmarksContextMenuTest, PasteWithSelection)
{
	CopiedBookmark copiedBookmark(*m_bookmarkToCopy);
	BookmarkHelper::CopyBookmarkItems(&m_clipboardStore, &m_bookmarkTree, { m_bookmarkToCopy },
		ClipboardAction::Copy);

	MenuViewFake menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate({ m_targetFolder->GetChildren()[0].get() });
	auto menu = BuildContextMenu(&menuView, &delegate);

	menuView.SelectItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_PASTE, false, false);
	ASSERT_EQ(m_targetFolder->GetChildren().size(), 3u);
	EXPECT_THAT(m_targetFolder->GetChildren()[1], Pointee(copiedBookmark));
}

TEST_F(OrganizeBookmarksContextMenuTest, SelectAll)
{
	MenuViewFake menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate;
	delegate.SetCanSelectAllItems(true);
	auto menu = BuildContextMenu(&menuView, &delegate);

	EXPECT_CALL(delegate, SelectAllItems());
	menuView.SelectItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_SELECT_ALL, false, false);
}

TEST_F(OrganizeBookmarksContextMenuTest, SelectionAfterTargetFolderDestroyed)
{
	MenuViewFake menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate;
	auto menu = BuildContextMenu(&menuView, &delegate);

	m_bookmarkTree.RemoveBookmarkItem(m_targetFolder);

	// The target folder was removed, so this call should have no effect, but should still be safe.
	EXPECT_CALL(delegate, CreateFolder).Times(0);
	menuView.SelectItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_NEW_FOLDER, false, false);
}

TEST_F(OrganizeBookmarksContextMenuTest, SelectionAfterParentFolderDestroyed)
{
	MenuViewFake menuView;
	OrganizeBookmarksContextMenuDelegateFake delegate;
	auto menu = BuildContextMenu(&menuView, &delegate);

	m_bookmarkTree.RemoveBookmarkItem(m_targetFolder->GetParent());

	// In this case, the target folder was implicitly removed (by removing the parent folder), but
	// this call should again be safe.
	EXPECT_CALL(delegate, CreateFolder).Times(0);
	menuView.SelectItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_NEW_FOLDER, false, false);
}
