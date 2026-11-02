// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/BookmarkItem.h"
#include <gtest/gtest.h>

using namespace testing;

TEST(BookmarkItemTest, CreateAndUpdate)
{
	BookmarkItem bookmarkFolder(L"Test folder");

	EXPECT_TRUE(bookmarkFolder.IsFolder());
	EXPECT_EQ(bookmarkFolder.GetName(), L"Test folder");

	BookmarkItem bookmark(L"Test bookmark", L"C:\\");

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
	BookmarkItem parentFolder(L"Test folder");
	auto bookmark = std::make_unique<BookmarkItem>(L"Test child bookmark", L"C:\\");
	auto rawBookmark = bookmark.get();

	parentFolder.AddChild(std::move(bookmark));

	EXPECT_EQ(parentFolder.GetChildren().size(), 1U);
	EXPECT_FALSE(parentFolder.HasChildFolder());
	EXPECT_EQ(parentFolder.GetChildAtIndex(0), rawBookmark);
	EXPECT_EQ(parentFolder.GetChildIndex(rawBookmark), 0U);

	EXPECT_EQ(rawBookmark->GetParent(), &parentFolder);

	auto folder = std::make_unique<BookmarkItem>(L"Test child folder");
	auto rawFolder = folder.get();

	parentFolder.AddChild(std::move(folder));

	EXPECT_EQ(parentFolder.GetChildren().size(), 2U);
	EXPECT_TRUE(parentFolder.HasChildFolder());
	EXPECT_EQ(parentFolder.GetChildAtIndex(1), rawFolder);
	EXPECT_EQ(parentFolder.GetChildIndex(rawFolder), 1U);

	EXPECT_EQ(rawFolder->GetParent(), &parentFolder);

	auto removedBookmark = parentFolder.RemoveChild(0);

	EXPECT_EQ(parentFolder.GetChildren().size(), 1U);
	EXPECT_TRUE(parentFolder.HasChildFolder());
	EXPECT_EQ(parentFolder.GetChildAtIndex(0), rawFolder);
	EXPECT_EQ(parentFolder.GetChildIndex(rawFolder), 0U);

	EXPECT_EQ(rawBookmark->GetParent(), nullptr);

	auto removedFolder = parentFolder.RemoveChild(0);

	EXPECT_EQ(parentFolder.GetChildren().size(), 0U);
	EXPECT_FALSE(parentFolder.HasChildFolder());

	EXPECT_EQ(rawFolder->GetParent(), nullptr);
}

TEST(BookmarkItemTest, VisitRecursively)
{
	auto folder = std::make_unique<BookmarkItem>(L"Folder");
	auto *childFolder = folder->AddChild(std::make_unique<BookmarkItem>(L"Child folder"));
	auto *bookmark1 = childFolder->AddChild(std::make_unique<BookmarkItem>(L"Bookmark 1", L"C:\\"));
	auto *grandchildFolder =
		childFolder->AddChild(std::make_unique<BookmarkItem>(L"Grandchild folder"));
	auto *bookmark2 =
		grandchildFolder->AddChild(std::make_unique<BookmarkItem>(L"Bookmark 2", L"D:\\"));
	auto *bookmark3 = folder->AddChild(std::make_unique<BookmarkItem>(L"Bookmark 3", L"E:\\"));

	std::vector<BookmarkItem *> items;
	folder->VisitRecursively([&items](BookmarkItem *currentItem) { items.push_back(currentItem); });
	EXPECT_THAT(items,
		ElementsAre(folder.get(), childFolder, bookmark1, grandchildFolder, bookmark2, bookmark3));
}
