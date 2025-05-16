// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/BookmarkClipboard.h"
#include "Bookmarks/BookmarkItem.h"
#include "SimulatedClipboardStore.h"
#include <gtest/gtest.h>

TEST(BookmarkClipboardTest, BookmarkReadWrite)
{
	BookmarkItems bookmarkItems;
	OwnedRefBookmarkItems ownedBookmarkItems;

	for (int i = 0; i < 10; i++)
	{
		auto currentBookmark = std::make_unique<BookmarkItem>(std::nullopt,
			L"Test bookmark " + std::to_wstring(i), L"C:\\Folder " + std::to_wstring(i));
		bookmarkItems.push_back(std::move(currentBookmark));
	}

	// Inserting the bookmark items into the vector above may result in the
	// vector being resized and existing references being invalidated.
	// Therefore, only retrieve references after the vector has been completely
	// constructed.
	for (auto &bookmarkItem : bookmarkItems)
	{
		ownedBookmarkItems.push_back(bookmarkItem);
	}

	SimulatedClipboardStore clipboardStore;
	BookmarkClipboard bookmarkClipboard(&clipboardStore);
	bool res = bookmarkClipboard.WriteBookmarks(ownedBookmarkItems);

	ASSERT_TRUE(res);

	auto clipboardItems = bookmarkClipboard.ReadBookmarks();

	ASSERT_EQ(clipboardItems.size(), bookmarkItems.size());

	int i = 0;

	for (auto &clipboardItem : clipboardItems)
	{
		EXPECT_EQ(clipboardItem->GetType(), bookmarkItems[i]->GetType());
		EXPECT_NE(clipboardItem->GetGUID(), bookmarkItems[i]->GetGUID());
		EXPECT_EQ(clipboardItem->GetOriginalGUID(), bookmarkItems[i]->GetGUID());
		EXPECT_EQ(clipboardItem->GetName(), bookmarkItems[i]->GetName());
		EXPECT_EQ(clipboardItem->GetLocation(), bookmarkItems[i]->GetLocation());

		i++;
	}
}
