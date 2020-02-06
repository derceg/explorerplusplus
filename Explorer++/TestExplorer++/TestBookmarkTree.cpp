// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Explorer++/BookmarkTree.h"

TEST(BookmarkTreeTest, BasicTests) {
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

TEST(BookmarkTreeTest, AddChildren) {
	BookmarkTree bookmarkTree;

	auto bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Test bookmark", L"C:\\");
	bookmarkTree.AddBookmarkItem(bookmarkTree.GetBookmarksMenuFolder(), std::move(bookmark), 0);

	EXPECT_EQ(bookmarkTree.GetBookmarksMenuFolder()->GetChildren().size(), 1);
	EXPECT_EQ(bookmarkTree.GetBookmarksToolbarFolder()->GetChildren().size(), 0);
	EXPECT_EQ(bookmarkTree.GetOtherBookmarksFolder()->GetChildren().size(), 0);

	for (int i = 0; i < 10; i++)
	{
		auto currentBookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Test bookmark " + std::to_wstring(i), L"C:\\");
		bookmarkTree.AddBookmarkItem(bookmarkTree.GetBookmarksToolbarFolder(), std::move(currentBookmark), i);
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

TEST(BookmarkTreeTest, MoveChildren) {
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

TEST(BookmarkTreeTest, RemoveChildren) {
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