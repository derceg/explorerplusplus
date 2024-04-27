// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/BookmarkDropper.h"
#include "BookmarkTreeHelper.h"
#include "Bookmarks/BookmarkDataExchange.h"
#include "Bookmarks/BookmarkTree.h"
#include "DragDropTestHelper.h"
#include "../Helper/ShellHelper.h"
#include <gtest/gtest.h>
#include <wil/com.h>

using namespace testing;

// Tests dropping a bookmark item.
class BookmarkDropperBookmarkItemTest : public Test
{
protected:
	BookmarkDropperBookmarkItemTest()
	{
		auto grandparentFolder =
			std::make_unique<BookmarkItem>(std::nullopt, L"Test grandparent folder", std::nullopt);
		m_rawGrandparentFolder = grandparentFolder.get();
		m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
			std::move(grandparentFolder), 0);

		auto parentFolder =
			std::make_unique<BookmarkItem>(std::nullopt, L"Test parent folder", std::nullopt);
		m_rawParentFolder = parentFolder.get();
		m_bookmarkTree.AddBookmarkItem(m_rawGrandparentFolder, std::move(parentFolder), 0);

		auto bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Test bookmark", L"C:\\");
		m_rawBookmark = bookmark.get();
		m_bookmarkTree.AddBookmarkItem(m_rawParentFolder, std::move(bookmark), 0);

		auto secondTopLevelFolder =
			std::make_unique<BookmarkItem>(std::nullopt, L"Second top-level folder", std::nullopt);
		m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
			std::move(secondTopLevelFolder), 1);

		auto &ownedPtr =
			m_rawGrandparentFolder->GetParent()->GetChildOwnedPtr(m_rawGrandparentFolder);
		m_dataObject = BookmarkDataExchange::CreateDataObject({ ownedPtr });

		m_dropper =
			std::make_unique<BookmarkDropper>(m_dataObject.get(), DROPEFFECT_MOVE, &m_bookmarkTree);
	}

	BookmarkTree m_bookmarkTree;
	std::unique_ptr<BookmarkDropper> m_dropper;
	winrt::com_ptr<IDataObject> m_dataObject;
	BookmarkItem *m_rawGrandparentFolder;
	BookmarkItem *m_rawParentFolder;
	BookmarkItem *m_rawBookmark;
};

TEST_F(BookmarkDropperBookmarkItemTest, DropEffect)
{
	// Drops on the root folder should be blocked.
	DWORD effect = m_dropper->GetDropEffect(m_bookmarkTree.GetRoot(), 0);
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_NONE));

	// An item can't be dropped at its current position.
	effect = m_dropper->GetDropEffect(m_bookmarkTree.GetBookmarksMenuFolder(), 0);
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_NONE));

	// It also can't be dropped after itself (which would really just be the same as being dropped
	// at its current position).
	effect = m_dropper->GetDropEffect(m_bookmarkTree.GetBookmarksMenuFolder(), 1);
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_NONE));

	// It should be possible to move the item to another position, though (such as after the item
	// that follows it).
	effect = m_dropper->GetDropEffect(m_bookmarkTree.GetBookmarksMenuFolder(), 2);
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_MOVE));

	effect = m_dropper->GetDropEffect(m_bookmarkTree.GetBookmarksToolbarFolder(), 0);
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_MOVE));

	effect = m_dropper->GetDropEffect(m_bookmarkTree.GetOtherBookmarksFolder(), 0);
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_MOVE));

	// An item can't be dropped on itself.
	effect = m_dropper->GetDropEffect(m_rawGrandparentFolder, 0);
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_NONE));

	// It also can't be dropped on one of its children.
	effect = m_dropper->GetDropEffect(m_rawParentFolder, 0);
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_NONE));

	// It shouldn't be possible to drop an item if the drop has been manually blocked.
	m_dropper->SetBlockDrop(true);
	effect = m_dropper->GetDropEffect(m_bookmarkTree.GetBookmarksToolbarFolder(), 0);
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_NONE));
}

TEST_F(BookmarkDropperBookmarkItemTest, DropOnRoot)
{
	DWORD effect = m_dropper->PerformDrop(m_bookmarkTree.GetRoot(), 0);
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_NONE));
	EXPECT_EQ(m_rawGrandparentFolder->GetParent(), m_bookmarkTree.GetBookmarksMenuFolder());
}

TEST_F(BookmarkDropperBookmarkItemTest, DropOnFolder)
{
	BookmarkTreeObserver observer;

	m_bookmarkTree.bookmarkItemMovedSignal.AddObserver(
		std::bind_front(&BookmarkTreeObserver::OnBookmarkItemMoved, &observer));

	EXPECT_CALL(observer,
		OnBookmarkItemMoved(m_rawGrandparentFolder, m_bookmarkTree.GetBookmarksMenuFolder(), 0,
			m_bookmarkTree.GetBookmarksToolbarFolder(), 0));

	DWORD effect = m_dropper->PerformDrop(m_bookmarkTree.GetBookmarksToolbarFolder(), 0);
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_MOVE));
}

TEST_F(BookmarkDropperBookmarkItemTest, DropWhenBlocked)
{
	m_dropper->SetBlockDrop(true);
	DWORD effect = m_dropper->PerformDrop(m_bookmarkTree.GetBookmarksToolbarFolder(), 0);
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_NONE));
}

// Tests dropping a shell item.
class BookmarkDropperShellItemTest : public TestWithParam<ShellItemType>
{
protected:
	void SetUp() override
	{
		m_itemName = L"item";
		m_itemPath = L"c:\\path\\to\\" + m_itemName;

		CreateShellDataObject(m_itemPath, GetParam(), m_dataObject);
	}

	BookmarkTree m_bookmarkTree;
	wil::com_ptr_nothrow<IDataObject> m_dataObject;
	std::wstring m_itemName;
	std::wstring m_itemPath;
};

TEST_P(BookmarkDropperShellItemTest, DropEffect)
{
	auto dropper =
		std::make_unique<BookmarkDropper>(m_dataObject.get(), DROPEFFECT_COPY, &m_bookmarkTree);
	DWORD effect = dropper->GetDropEffect(m_bookmarkTree.GetBookmarksMenuFolder(), 0);
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_COPY));

	dropper =
		std::make_unique<BookmarkDropper>(m_dataObject.get(), DROPEFFECT_LINK, &m_bookmarkTree);
	effect = dropper->GetDropEffect(m_bookmarkTree.GetBookmarksMenuFolder(), 0);
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_LINK));

	// It's not possible to move shell items into the bookmarks tree (the only items that can be
	// moved are actual bookmarks).
	dropper =
		std::make_unique<BookmarkDropper>(m_dataObject.get(), DROPEFFECT_MOVE, &m_bookmarkTree);
	effect = dropper->GetDropEffect(m_bookmarkTree.GetBookmarksMenuFolder(), 0);
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_NONE));
}

TEST_P(BookmarkDropperShellItemTest, Drop)
{
	auto dropper =
		std::make_unique<BookmarkDropper>(m_dataObject.get(), DROPEFFECT_COPY, &m_bookmarkTree);

	DWORD effect = dropper->PerformDrop(m_bookmarkTree.GetBookmarksMenuFolder(), 0);
	ASSERT_EQ(effect, static_cast<DWORD>(DROPEFFECT_COPY));
	ASSERT_EQ(m_bookmarkTree.GetBookmarksMenuFolder()->GetChildren().size(), 1U);

	auto bookmarkItem = m_bookmarkTree.GetBookmarksMenuFolder()->GetChildren()[0].get();
	EXPECT_TRUE(bookmarkItem->IsBookmark());
	EXPECT_EQ(bookmarkItem->GetName(), m_itemName);
	EXPECT_THAT(bookmarkItem->GetLocation(), StrCaseEq(m_itemPath));
}

// It should be possible to drop both files and folders. The BookmarkDropperShellItemTest suite will
// be run for both a file item and a folder item.
INSTANTIATE_TEST_SUITE_P(FileAndFolder, BookmarkDropperShellItemTest,
	Values(ShellItemType::File, ShellItemType::Folder));

// Tests dropping data which can't be converted to a bookmark (in this case, text).
class BookmarkDropperInvalidDataTest : public Test
{
protected:
	void SetUp() override
	{
		CreateTextDataObject(L"Test", m_dataObject);
		m_dropper =
			std::make_unique<BookmarkDropper>(m_dataObject.get(), DROPEFFECT_COPY, &m_bookmarkTree);
	}

	BookmarkTree m_bookmarkTree;
	std::unique_ptr<BookmarkDropper> m_dropper;
	winrt::com_ptr<IDataObject> m_dataObject;
};

TEST_F(BookmarkDropperInvalidDataTest, DropEffect)
{
	// It's not possible to extract any bookmarks items from the drop data, so it shouldn't be
	// possible to drop anywhere.
	DWORD effect = m_dropper->GetDropEffect(m_bookmarkTree.GetRoot(), 0);
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_NONE));

	effect = m_dropper->GetDropEffect(m_bookmarkTree.GetBookmarksMenuFolder(), 0);
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_NONE));

	effect = m_dropper->GetDropEffect(m_bookmarkTree.GetBookmarksToolbarFolder(), 0);
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_NONE));

	effect = m_dropper->GetDropEffect(m_bookmarkTree.GetOtherBookmarksFolder(), 0);
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_NONE));
}

TEST_F(BookmarkDropperInvalidDataTest, Drop)
{
	DWORD effect = m_dropper->PerformDrop(m_bookmarkTree.GetBookmarksMenuFolder(), 0);
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_NONE));
}
