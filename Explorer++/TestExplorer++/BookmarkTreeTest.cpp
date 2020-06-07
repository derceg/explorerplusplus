// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "Bookmarks/BookmarkTree.h"
#include "BookmarkTreeHelper.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class BookmarkTreeObserverTest : public Test
{
protected:
	BookmarkTreeObserverTest()
	{
		auto folder = std::make_unique<BookmarkItem>(std::nullopt, L"Test folder", std::nullopt);
		m_rawFolder = folder.get();
		m_bookmarkTree.AddBookmarkItem(
			m_bookmarkTree.GetBookmarksMenuFolder(), std::move(folder), 0);

		auto bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Test bookmark", L"C:\\");
		m_rawBookmark = bookmark.get();
		m_bookmarkTree.AddBookmarkItem(m_rawFolder, std::move(bookmark), 0);
	}

	BookmarkTree m_bookmarkTree;
	BookmarkTreeObserver m_observer;

	BookmarkItem *m_rawFolder;
	BookmarkItem *m_rawBookmark;
};

TEST(BookmarkTreeTest, BasicTests)
{
	BookmarkTree bookmarkTree;

	EXPECT_FALSE(bookmarkTree.CanAddChildren(bookmarkTree.GetRoot()));
	EXPECT_TRUE(bookmarkTree.CanAddChildren(bookmarkTree.GetBookmarksMenuFolder()));
	EXPECT_TRUE(bookmarkTree.CanAddChildren(bookmarkTree.GetBookmarksToolbarFolder()));
	EXPECT_TRUE(bookmarkTree.CanAddChildren(bookmarkTree.GetOtherBookmarksFolder()));

	EXPECT_TRUE(bookmarkTree.IsPermanentNode(bookmarkTree.GetRoot()));
	EXPECT_TRUE(bookmarkTree.IsPermanentNode(bookmarkTree.GetBookmarksMenuFolder()));
	EXPECT_TRUE(bookmarkTree.IsPermanentNode(bookmarkTree.GetBookmarksToolbarFolder()));
	EXPECT_TRUE(bookmarkTree.IsPermanentNode(bookmarkTree.GetOtherBookmarksFolder()));

	auto bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Test bookmark", L"C:\\");
	auto rawBookmark = bookmark.get();
	bookmarkTree.AddBookmarkItem(bookmarkTree.GetBookmarksMenuFolder(), std::move(bookmark), 0);

	EXPECT_FALSE(bookmarkTree.IsPermanentNode(rawBookmark));

	auto folder = std::make_unique<BookmarkItem>(std::nullopt, L"Test folder", std::nullopt);
	auto rawFolder = folder.get();
	bookmarkTree.AddBookmarkItem(bookmarkTree.GetBookmarksMenuFolder(), std::move(folder), 1);

	EXPECT_TRUE(bookmarkTree.CanAddChildren(rawFolder));
	EXPECT_FALSE(bookmarkTree.IsPermanentNode(rawFolder));
}

TEST(BookmarkTreeTest, AddChildren)
{
	BookmarkTree bookmarkTree;

	auto bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Test bookmark", L"C:\\");
	bookmarkTree.AddBookmarkItem(bookmarkTree.GetBookmarksMenuFolder(), std::move(bookmark), 0);

	EXPECT_EQ(bookmarkTree.GetBookmarksMenuFolder()->GetChildren().size(), 1);
	EXPECT_EQ(bookmarkTree.GetBookmarksToolbarFolder()->GetChildren().size(), 0);
	EXPECT_EQ(bookmarkTree.GetOtherBookmarksFolder()->GetChildren().size(), 0);

	for (int i = 0; i < 10; i++)
	{
		auto currentBookmark = std::make_unique<BookmarkItem>(
			std::nullopt, L"Test bookmark " + std::to_wstring(i), L"C:\\");
		bookmarkTree.AddBookmarkItem(
			bookmarkTree.GetBookmarksToolbarFolder(), std::move(currentBookmark), i);
	}

	EXPECT_EQ(bookmarkTree.GetBookmarksMenuFolder()->GetChildren().size(), 1);
	EXPECT_EQ(bookmarkTree.GetBookmarksToolbarFolder()->GetChildren().size(), 10);
	EXPECT_EQ(bookmarkTree.GetOtherBookmarksFolder()->GetChildren().size(), 0);

	for (int i = 0; i < 10; i++)
	{
		auto &currentBookmark = bookmarkTree.GetBookmarksToolbarFolder()->GetChildren().at(i);

		EXPECT_EQ(currentBookmark->GetName(), L"Test bookmark " + std::to_wstring(i));
	}

	bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Test bookmark", L"C:\\");
	auto rawBookmark = bookmark.get();
	bookmarkTree.AddBookmarkItem(bookmarkTree.GetOtherBookmarksFolder(), std::move(bookmark), 100);

	EXPECT_EQ(bookmarkTree.GetBookmarksMenuFolder()->GetChildren().size(), 1);
	EXPECT_EQ(bookmarkTree.GetBookmarksToolbarFolder()->GetChildren().size(), 10);
	EXPECT_EQ(bookmarkTree.GetOtherBookmarksFolder()->GetChildren().size(), 1);

	EXPECT_EQ(bookmarkTree.GetOtherBookmarksFolder()->GetChildIndex(rawBookmark), 0);
}

TEST(BookmarkTreeTest, MoveChildren)
{
	BookmarkTree bookmarkTree;

	auto bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Test bookmark", L"C:\\");
	auto rawBookmark = bookmark.get();
	bookmarkTree.AddBookmarkItem(bookmarkTree.GetBookmarksMenuFolder(), std::move(bookmark), 0);

	EXPECT_EQ(bookmarkTree.GetBookmarksMenuFolder()->GetChildren().size(), 1);
	EXPECT_EQ(bookmarkTree.GetBookmarksToolbarFolder()->GetChildren().size(), 0);
	EXPECT_EQ(bookmarkTree.GetOtherBookmarksFolder()->GetChildren().size(), 0);

	bookmarkTree.MoveBookmarkItem(rawBookmark, bookmarkTree.GetBookmarksToolbarFolder(), 0);

	EXPECT_EQ(bookmarkTree.GetBookmarksMenuFolder()->GetChildren().size(), 0);
	EXPECT_EQ(bookmarkTree.GetBookmarksToolbarFolder()->GetChildren().size(), 1);
	EXPECT_EQ(bookmarkTree.GetOtherBookmarksFolder()->GetChildren().size(), 0);
}

TEST(BookmarkTreeTest, RemoveChildren)
{
	BookmarkTree bookmarkTree;

	auto bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Test bookmark", L"C:\\");
	auto rawBookmark = bookmark.get();
	bookmarkTree.AddBookmarkItem(bookmarkTree.GetBookmarksMenuFolder(), std::move(bookmark), 0);

	EXPECT_EQ(bookmarkTree.GetBookmarksMenuFolder()->GetChildren().size(), 1);
	EXPECT_EQ(bookmarkTree.GetBookmarksToolbarFolder()->GetChildren().size(), 0);
	EXPECT_EQ(bookmarkTree.GetOtherBookmarksFolder()->GetChildren().size(), 0);

	bookmarkTree.RemoveBookmarkItem(rawBookmark);

	EXPECT_EQ(bookmarkTree.GetBookmarksMenuFolder()->GetChildren().size(), 0);
	EXPECT_EQ(bookmarkTree.GetBookmarksToolbarFolder()->GetChildren().size(), 0);
	EXPECT_EQ(bookmarkTree.GetOtherBookmarksFolder()->GetChildren().size(), 0);
}

TEST_F(BookmarkTreeObserverTest, Add)
{
	m_bookmarkTree.bookmarkItemAddedSignal.AddObserver(
		std::bind(&BookmarkTreeObserver::OnBookmarkItemAdded, &m_observer, std::placeholders::_1,
			std::placeholders::_2));

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
		std::bind(&BookmarkTreeObserver::OnBookmarkItemUpdated, &m_observer, std::placeholders::_1,
			std::placeholders::_2));

	EXPECT_CALL(
		m_observer, OnBookmarkItemUpdated(Ref(*m_rawFolder), BookmarkItem::PropertyType::Name));
	m_rawFolder->SetName(L"New name");

	EXPECT_CALL(m_observer,
		OnBookmarkItemUpdated(Ref(*m_rawFolder), BookmarkItem::PropertyType::DateModified));

	FILETIME dateModified;
	GetSystemTimeAsFileTime(&dateModified);
	m_rawFolder->SetDateModified(dateModified);

	EXPECT_CALL(
		m_observer, OnBookmarkItemUpdated(Ref(*m_rawBookmark), BookmarkItem::PropertyType::Name));
	m_rawBookmark->SetName(L"New name");

	EXPECT_CALL(m_observer,
		OnBookmarkItemUpdated(Ref(*m_rawBookmark), BookmarkItem::PropertyType::Location));
	m_rawBookmark->SetLocation(L"D:\\");
}

TEST_F(BookmarkTreeObserverTest, Move)
{
	m_bookmarkTree.bookmarkItemMovedSignal.AddObserver(
		std::bind(&BookmarkTreeObserver::OnBookmarkItemMoved, &m_observer, std::placeholders::_1,
			std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
			std::placeholders::_5));

	EXPECT_CALL(m_observer,
		OnBookmarkItemMoved(m_rawFolder, m_bookmarkTree.GetBookmarksMenuFolder(), 0,
			m_bookmarkTree.GetBookmarksToolbarFolder(), 0));
	m_bookmarkTree.MoveBookmarkItem(m_rawFolder, m_bookmarkTree.GetBookmarksToolbarFolder(), 0);

	EXPECT_CALL(m_observer,
		OnBookmarkItemMoved(
			m_rawBookmark, m_rawFolder, 0, m_bookmarkTree.GetBookmarksToolbarFolder(), 1));
	m_bookmarkTree.MoveBookmarkItem(m_rawBookmark, m_bookmarkTree.GetBookmarksToolbarFolder(), 1);
}

TEST_F(BookmarkTreeObserverTest, Remove)
{
	m_bookmarkTree.bookmarkItemPreRemovalSignal.AddObserver(std::bind(
		&BookmarkTreeObserver::OnBookmarkItemPreRemoval, &m_observer, std::placeholders::_1));

	m_bookmarkTree.bookmarkItemRemovedSignal.AddObserver(std::bind(
		&BookmarkTreeObserver::OnBookmarkItemRemoved, &m_observer, std::placeholders::_1));

	EXPECT_CALL(m_observer, OnBookmarkItemPreRemoval(Ref(*m_rawBookmark)));
	EXPECT_CALL(m_observer, OnBookmarkItemRemoved(m_rawBookmark->GetGUID()));
	m_bookmarkTree.RemoveBookmarkItem(m_rawBookmark);

	EXPECT_CALL(m_observer, OnBookmarkItemPreRemoval(Ref(*m_rawFolder)));
	EXPECT_CALL(m_observer, OnBookmarkItemRemoved(m_rawFolder->GetGUID()));
	m_bookmarkTree.RemoveBookmarkItem(m_rawFolder);
}