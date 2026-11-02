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

	for (int i = 0; i < 10; i++)
	{
		auto currentBookmark = std::make_unique<BookmarkItem>(
			L"Test bookmark " + std::to_wstring(i), L"C:\\Folder " + std::to_wstring(i));
		bookmarkItems.push_back(std::move(currentBookmark));
	}

	RawBookmarkItems rawBookmarkItems;

	for (const auto &bookmarkItem : bookmarkItems)
	{
		rawBookmarkItems.push_back(bookmarkItem.get());
	}

	SimulatedClipboardStore clipboardStore;
	BookmarkClipboard bookmarkClipboard(&clipboardStore);
	bool res = bookmarkClipboard.WriteBookmarks(rawBookmarkItems);
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
