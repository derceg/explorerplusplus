// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "Bookmarks/BookmarkTree.h"
#include <gtest/gtest.h>

void CompareFolders(const BookmarkItem *firstFolder, const BookmarkItem *secondFolder);
void CompareBookmarks(const BookmarkItem *firstBookmark, const BookmarkItem *secondBookmark);

void BuildV1BasicLoadReferenceTree(BookmarkTree *bookmarkTree)
{
	auto bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Root", L"C:\\");
	bookmarkTree->AddBookmarkItem(
		bookmarkTree->GetBookmarksToolbarFolder(), std::move(bookmark), 0);

	auto folder = std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt);
	auto *rawFolder = bookmarkTree->AddBookmarkItem(
		bookmarkTree->GetBookmarksToolbarFolder(), std::move(folder), 1);
	ASSERT_TRUE(rawFolder);

	bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Windows", L"C:\\Windows");
	bookmarkTree->AddBookmarkItem(rawFolder, std::move(bookmark), 0);

	folder = std::make_unique<BookmarkItem>(std::nullopt, L"Folder 2", std::nullopt);
	bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksMenuFolder(), std::move(folder), 0);
}

void BuildV1NestedShowOnToolbarLoadReferenceTree(BookmarkTree *bookmarkTree)
{
	auto bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Root", L"C:\\");
	bookmarkTree->AddBookmarkItem(
		bookmarkTree->GetBookmarksToolbarFolder(), std::move(bookmark), 0);

	auto folder = std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt);
	auto *rawFolder =
		bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksMenuFolder(), std::move(folder), 0);
	ASSERT_TRUE(rawFolder);

	bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Windows", L"C:\\Windows");
	bookmarkTree->AddBookmarkItem(rawFolder, std::move(bookmark), 0);

	folder = std::make_unique<BookmarkItem>(std::nullopt, L"Folder 2", std::nullopt);
	bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksMenuFolder(), std::move(folder), 1);
}

void CompareBookmarkTrees(const BookmarkTree *firstTree, const BookmarkTree *secondTree)
{
	CompareFolders(firstTree->GetBookmarksMenuFolder(), secondTree->GetBookmarksMenuFolder());
	CompareFolders(firstTree->GetBookmarksToolbarFolder(), secondTree->GetBookmarksToolbarFolder());
	CompareFolders(firstTree->GetOtherBookmarksFolder(), secondTree->GetOtherBookmarksFolder());
}

void CompareFolders(const BookmarkItem *firstFolder, const BookmarkItem *secondFolder)
{
	EXPECT_EQ(firstFolder->GetName(), secondFolder->GetName());

	auto &firstChildren = firstFolder->GetChildren();
	auto &secondChildren = secondFolder->GetChildren();
	ASSERT_EQ(firstChildren.size(), secondChildren.size());

	for (size_t i = 0; i < firstChildren.size(); i++)
	{
		auto *firstCurrent = firstChildren[i].get();
		auto *secondCurrent = secondChildren[i].get();

		ASSERT_EQ(firstCurrent->GetType(), secondCurrent->GetType());

		if (firstCurrent->IsFolder())
		{
			CompareFolders(firstCurrent, secondCurrent);
		}
		else
		{
			CompareBookmarks(firstCurrent, secondCurrent);
		}
	}
}

void CompareBookmarks(const BookmarkItem *firstBookmark, const BookmarkItem *secondBookmark)
{
	EXPECT_EQ(firstBookmark->GetName(), secondBookmark->GetName());
	EXPECT_EQ(firstBookmark->GetLocation(), secondBookmark->GetLocation());
}