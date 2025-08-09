// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/BookmarkTree.h"
#include "BookmarkTreeHelper.h"
#include "Bookmarks/BookmarkHelper.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <ranges>

using namespace testing;

class BookmarkTreeTest : public Test
{
protected:
	BookmarkTree m_bookmarkTree;
};

TEST_F(BookmarkTreeTest, BasicTests)
{
	EXPECT_FALSE(m_bookmarkTree.CanAddChildren(m_bookmarkTree.GetRoot()));
	EXPECT_TRUE(m_bookmarkTree.CanAddChildren(m_bookmarkTree.GetBookmarksMenuFolder()));
	EXPECT_TRUE(m_bookmarkTree.CanAddChildren(m_bookmarkTree.GetBookmarksToolbarFolder()));
	EXPECT_TRUE(m_bookmarkTree.CanAddChildren(m_bookmarkTree.GetOtherBookmarksFolder()));

	EXPECT_TRUE(m_bookmarkTree.IsPermanentNode(m_bookmarkTree.GetRoot()));
	EXPECT_TRUE(m_bookmarkTree.IsPermanentNode(m_bookmarkTree.GetBookmarksMenuFolder()));
	EXPECT_TRUE(m_bookmarkTree.IsPermanentNode(m_bookmarkTree.GetBookmarksToolbarFolder()));
	EXPECT_TRUE(m_bookmarkTree.IsPermanentNode(m_bookmarkTree.GetOtherBookmarksFolder()));

	auto bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Test bookmark", L"C:\\");
	auto rawBookmark = bookmark.get();
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(), std::move(bookmark), 0);

	EXPECT_FALSE(m_bookmarkTree.IsPermanentNode(rawBookmark));

	auto folder = std::make_unique<BookmarkItem>(std::nullopt, L"Test folder", std::nullopt);
	auto rawFolder = folder.get();
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(), std::move(folder), 1);

	EXPECT_TRUE(m_bookmarkTree.CanAddChildren(rawFolder));
	EXPECT_FALSE(m_bookmarkTree.IsPermanentNode(rawFolder));
}

TEST_F(BookmarkTreeTest, AddChildren)
{
	auto bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Test bookmark", L"C:\\");
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(), std::move(bookmark), 0);

	EXPECT_EQ(m_bookmarkTree.GetBookmarksMenuFolder()->GetChildren().size(), 1U);
	EXPECT_EQ(m_bookmarkTree.GetBookmarksToolbarFolder()->GetChildren().size(), 0U);
	EXPECT_EQ(m_bookmarkTree.GetOtherBookmarksFolder()->GetChildren().size(), 0U);

	for (int i = 0; i < 10; i++)
	{
		auto currentBookmark = std::make_unique<BookmarkItem>(std::nullopt,
			L"Test bookmark " + std::to_wstring(i), L"C:\\");
		m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
			std::move(currentBookmark), i);
	}

	EXPECT_EQ(m_bookmarkTree.GetBookmarksMenuFolder()->GetChildren().size(), 1U);
	EXPECT_EQ(m_bookmarkTree.GetBookmarksToolbarFolder()->GetChildren().size(), 10U);
	EXPECT_EQ(m_bookmarkTree.GetOtherBookmarksFolder()->GetChildren().size(), 0U);

	for (int i = 0; i < 10; i++)
	{
		auto &currentBookmark = m_bookmarkTree.GetBookmarksToolbarFolder()->GetChildren().at(i);

		EXPECT_EQ(currentBookmark->GetName(), L"Test bookmark " + std::to_wstring(i));
	}

	bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Test bookmark", L"C:\\");
	auto rawBookmark = bookmark.get();
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetOtherBookmarksFolder(), std::move(bookmark),
		100);

	EXPECT_EQ(m_bookmarkTree.GetBookmarksMenuFolder()->GetChildren().size(), 1U);
	EXPECT_EQ(m_bookmarkTree.GetBookmarksToolbarFolder()->GetChildren().size(), 10U);
	EXPECT_EQ(m_bookmarkTree.GetOtherBookmarksFolder()->GetChildren().size(), 1U);

	EXPECT_EQ(m_bookmarkTree.GetOtherBookmarksFolder()->GetChildIndex(rawBookmark), 0U);
}

TEST_F(BookmarkTreeTest, AddChildrenAtEnd)
{
	auto *parentFolder = m_bookmarkTree.GetOtherBookmarksFolder();

	std::vector<BookmarkItem *> addedItems;

	// If no index is provided, the bookmark item should be added at the end.
	addedItems.push_back(m_bookmarkTree.AddBookmarkItem(parentFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Item 1", L"C:\\")));
	addedItems.push_back(m_bookmarkTree.AddBookmarkItem(parentFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Item 2", std::nullopt)));
	addedItems.push_back(m_bookmarkTree.AddBookmarkItem(parentFolder,
		std::make_unique<BookmarkItem>(std::nullopt, L"Item 3", L"D:\\")));

	EXPECT_TRUE(std::ranges::equal(parentFolder->GetChildren()
			| std::views::transform([](const auto &bookmark) { return bookmark.get(); }),
		addedItems));
}

TEST_F(BookmarkTreeTest, MoveChildren)
{
	auto bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Test bookmark", L"C:\\");
	auto rawBookmark = bookmark.get();
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(), std::move(bookmark), 0);

	EXPECT_EQ(m_bookmarkTree.GetBookmarksMenuFolder()->GetChildren().size(), 1U);
	EXPECT_EQ(m_bookmarkTree.GetBookmarksToolbarFolder()->GetChildren().size(), 0U);
	EXPECT_EQ(m_bookmarkTree.GetOtherBookmarksFolder()->GetChildren().size(), 0U);

	m_bookmarkTree.MoveBookmarkItem(rawBookmark, m_bookmarkTree.GetBookmarksToolbarFolder(), 0);

	EXPECT_EQ(m_bookmarkTree.GetBookmarksMenuFolder()->GetChildren().size(), 0U);
	EXPECT_EQ(m_bookmarkTree.GetBookmarksToolbarFolder()->GetChildren().size(), 1U);
	EXPECT_EQ(m_bookmarkTree.GetOtherBookmarksFolder()->GetChildren().size(), 0U);
}

TEST_F(BookmarkTreeTest, RemoveChildren)
{
	auto bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Test bookmark", L"C:\\");
	auto rawBookmark = bookmark.get();
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(), std::move(bookmark), 0);

	EXPECT_EQ(m_bookmarkTree.GetBookmarksMenuFolder()->GetChildren().size(), 1U);
	EXPECT_EQ(m_bookmarkTree.GetBookmarksToolbarFolder()->GetChildren().size(), 0U);
	EXPECT_EQ(m_bookmarkTree.GetOtherBookmarksFolder()->GetChildren().size(), 0U);

	m_bookmarkTree.RemoveBookmarkItem(rawBookmark);

	EXPECT_EQ(m_bookmarkTree.GetBookmarksMenuFolder()->GetChildren().size(), 0U);
	EXPECT_EQ(m_bookmarkTree.GetBookmarksToolbarFolder()->GetChildren().size(), 0U);
	EXPECT_EQ(m_bookmarkTree.GetOtherBookmarksFolder()->GetChildren().size(), 0U);
}

TEST_F(BookmarkTreeTest, RemovePermanentNode)
{
	auto *bookmarkFolder = m_bookmarkTree.GetBookmarksToolbarFolder();
	auto guid = bookmarkFolder->GetGUID();
	m_bookmarkTree.RemoveBookmarkItem(bookmarkFolder);

	// Attempting to remove a permanent node should have no effect (i.e. the folder shouldn't be
	// removed).
	auto *retrievedBookmarkFolder = BookmarkHelper::GetBookmarkItemById(&m_bookmarkTree, guid);
	EXPECT_EQ(retrievedBookmarkFolder, bookmarkFolder);
}

class BookmarkTreeObserverTest : public Test
{
protected:
	BookmarkTreeObserverTest()
	{
		auto folder = std::make_unique<BookmarkItem>(std::nullopt, L"Test folder", std::nullopt);
		m_rawFolder = folder.get();
		m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(), std::move(folder),
			0);

		auto bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Test bookmark", L"C:\\");
		m_rawBookmark = bookmark.get();
		m_bookmarkTree.AddBookmarkItem(m_rawFolder, std::move(bookmark), 0);
	}

	BookmarkTree m_bookmarkTree;
	BookmarkTreeObserver m_observer;

	BookmarkItem *m_rawFolder;
	BookmarkItem *m_rawBookmark;
};

TEST_F(BookmarkTreeObserverTest, Add)
{
	m_bookmarkTree.bookmarkItemAddedSignal.AddObserver(
		std::bind_front(&BookmarkTreeObserver::OnBookmarkItemAdded, &m_observer));

	auto folder = std::make_unique<BookmarkItem>(std::nullopt, L"Test folder", std::nullopt);
	auto rawFolder = folder.get();

	EXPECT_CALL(m_observer, OnBookmarkItemAdded(Ref(*rawFolder), 0));
	m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(), std::move(folder), 0);

	// Adding an observer to the bookmark tree should mean that events affecting
	// any bookmark item in the tree are captured (e.g. adding a bookmark
	// anywhere in the tree should result in a change notification).
	auto bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Test bookmark", L"C:\\");
	auto rawBookmark = bookmark.get();

	EXPECT_CALL(m_observer, OnBookmarkItemAdded(Ref(*rawBookmark), 0));
	m_bookmarkTree.AddBookmarkItem(rawFolder, std::move(bookmark), 0);
}

TEST_F(BookmarkTreeObserverTest, Update)
{
	m_bookmarkTree.bookmarkItemUpdatedSignal.AddObserver(
		std::bind_front(&BookmarkTreeObserver::OnBookmarkItemUpdated, &m_observer));

	EXPECT_CALL(m_observer,
		OnBookmarkItemUpdated(Ref(*m_rawFolder), BookmarkItem::PropertyType::Name));
	m_rawFolder->SetName(L"New name");

	EXPECT_CALL(m_observer,
		OnBookmarkItemUpdated(Ref(*m_rawFolder), BookmarkItem::PropertyType::DateModified));

	FILETIME dateModified;
	GetSystemTimeAsFileTime(&dateModified);
	m_rawFolder->SetDateModified(dateModified);

	EXPECT_CALL(m_observer,
		OnBookmarkItemUpdated(Ref(*m_rawBookmark), BookmarkItem::PropertyType::Name));
	m_rawBookmark->SetName(L"New name");

	EXPECT_CALL(m_observer,
		OnBookmarkItemUpdated(Ref(*m_rawBookmark), BookmarkItem::PropertyType::Location));
	m_rawBookmark->SetLocation(L"D:\\");
}

TEST_F(BookmarkTreeObserverTest, Move)
{
	m_bookmarkTree.bookmarkItemMovedSignal.AddObserver(
		std::bind_front(&BookmarkTreeObserver::OnBookmarkItemMoved, &m_observer));

	EXPECT_CALL(m_observer,
		OnBookmarkItemMoved(m_rawFolder, m_bookmarkTree.GetBookmarksMenuFolder(), 0,
			m_bookmarkTree.GetBookmarksToolbarFolder(), 0));
	m_bookmarkTree.MoveBookmarkItem(m_rawFolder, m_bookmarkTree.GetBookmarksToolbarFolder(), 0);

	EXPECT_CALL(m_observer,
		OnBookmarkItemMoved(m_rawBookmark, m_rawFolder, 0,
			m_bookmarkTree.GetBookmarksToolbarFolder(), 1));
	m_bookmarkTree.MoveBookmarkItem(m_rawBookmark, m_bookmarkTree.GetBookmarksToolbarFolder(), 1);
}

TEST_F(BookmarkTreeObserverTest, Remove)
{
	m_bookmarkTree.bookmarkItemPreRemovalSignal.AddObserver(
		std::bind_front(&BookmarkTreeObserver::OnBookmarkItemPreRemoval, &m_observer));

	m_bookmarkTree.bookmarkItemRemovedSignal.AddObserver(
		std::bind_front(&BookmarkTreeObserver::OnBookmarkItemRemoved, &m_observer));

	EXPECT_CALL(m_observer, OnBookmarkItemPreRemoval(Ref(*m_rawBookmark)));
	EXPECT_CALL(m_observer, OnBookmarkItemRemoved(m_rawBookmark->GetGUID()));
	m_bookmarkTree.RemoveBookmarkItem(m_rawBookmark);

	EXPECT_CALL(m_observer, OnBookmarkItemPreRemoval(Ref(*m_rawFolder)));
	EXPECT_CALL(m_observer, OnBookmarkItemRemoved(m_rawFolder->GetGUID()));
	m_bookmarkTree.RemoveBookmarkItem(m_rawFolder);
}
