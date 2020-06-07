// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "Bookmarks/BookmarkItem.h"
#include <gtest/gtest.h>

TEST(BookmarkItemTest, CreateAndUpdate)
{
	BookmarkItem bookmarkFolder(std::nullopt, L"Test folder", std::nullopt);

	EXPECT_TRUE(bookmarkFolder.IsFolder());
	EXPECT_EQ(bookmarkFolder.GetName(), L"Test folder");

	BookmarkItem bookmark(std::nullopt, L"Test bookmark", L"C:\\");

	EXPECT_TRUE(bookmark.IsBookmark());
	EXPECT_NE(bookmark.GetGUID(), bookmarkFolder.GetGUID());
	EXPECT_EQ(bookmark.GetName(), L"Test bookmark");
	EXPECT_EQ(bookmark.GetLocation(), L"C:\\");

	bookmark.SetName(L"New bookmark name");
	EXPECT_EQ(bookmark.GetName(), L"New bookmark name");

	bookmark.SetLocation(L"D:\\");
	EXPECT_EQ(bookmark.GetLocation(), L"D:\\");
}

TEST(BookmarkItemTest, AddRemoveChildren)
{
	BookmarkItem parentFolder(std::nullopt, L"Test folder", std::nullopt);
	auto bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Test child bookmark", L"C:\\");
	auto rawBookmark = bookmark.get();

	parentFolder.AddChild(std::move(bookmark));

	EXPECT_EQ(parentFolder.GetChildren().size(), 1);
	EXPECT_FALSE(parentFolder.HasChildFolder());
	EXPECT_EQ(parentFolder.GetChildIndex(rawBookmark), 0);

	EXPECT_EQ(rawBookmark->GetParent(), &parentFolder);

	auto folder = std::make_unique<BookmarkItem>(std::nullopt, L"Test child folder", std::nullopt);
	auto rawFolder = folder.get();

	parentFolder.AddChild(std::move(folder));

	EXPECT_EQ(parentFolder.GetChildren().size(), 2);
	EXPECT_TRUE(parentFolder.HasChildFolder());
	EXPECT_EQ(parentFolder.GetChildIndex(rawFolder), 1);

	EXPECT_EQ(rawFolder->GetParent(), &parentFolder);

	auto removedBookmark = parentFolder.RemoveChild(0);

	EXPECT_EQ(parentFolder.GetChildren().size(), 1);
	EXPECT_TRUE(parentFolder.HasChildFolder());
	EXPECT_THROW(parentFolder.GetChildIndex(rawBookmark), std::invalid_argument);
	EXPECT_EQ(parentFolder.GetChildIndex(rawFolder), 0);

	EXPECT_EQ(rawBookmark->GetParent(), nullptr);

	auto removedFolder = parentFolder.RemoveChild(0);

	EXPECT_EQ(parentFolder.GetChildren().size(), 0);
	EXPECT_FALSE(parentFolder.HasChildFolder());
	EXPECT_THROW(parentFolder.GetChildIndex(rawFolder), std::invalid_argument);

	EXPECT_EQ(rawFolder->GetParent(), nullptr);
}