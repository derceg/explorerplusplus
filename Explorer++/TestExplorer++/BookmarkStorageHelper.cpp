// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/BookmarkTree.h"
#include <gtest/gtest.h>

void CompareFolders(const BookmarkItem *firstFolder, const BookmarkItem *secondFolder,
	bool compareGuids);
void CompareBookmarks(const BookmarkItem *firstBookmark, const BookmarkItem *secondBookmark,
	bool compareGuids);

void BuildV2LoadSaveReferenceTree(BookmarkTree *bookmarkTree)
{
	auto folder = std::make_unique<BookmarkItem>(L"C974D322-89F3-402E-ADE7-6AB447DB54ED",
		L"Menu Folder 1", std::nullopt);
	auto *rawFolder =
		bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksMenuFolder(), std::move(folder), 0);
	ASSERT_TRUE(rawFolder);

	auto subFolder = std::make_unique<BookmarkItem>(L"F4382A95-DCC2-41D3-8675-98243D9956A1",
		L"Sub Folder 1", std::nullopt);
	auto *rawSubFolder = bookmarkTree->AddBookmarkItem(rawFolder, std::move(subFolder), 0);
	ASSERT_TRUE(rawSubFolder);

	auto bookmark = std::make_unique<BookmarkItem>(L"CC620AF8-6761-4A1C-A7CD-CD23F5054FBD",
		L"Second drive", L"D:\\");
	bookmarkTree->AddBookmarkItem(rawSubFolder, std::move(bookmark), 0);

	folder = std::make_unique<BookmarkItem>(L"509F37C6-18CA-44BD-890A-53462529B71D",
		L"Menu Folder 2", std::nullopt);
	rawFolder =
		bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksMenuFolder(), std::move(folder), 1);
	ASSERT_TRUE(rawFolder);

	bookmark = std::make_unique<BookmarkItem>(L"3AD29787-DE86-4149-8A5E-E615E5232787", L"Public",
		L"C:\\Users\\Public");
	bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksMenuFolder(), std::move(bookmark), 2);

	bookmark =
		std::make_unique<BookmarkItem>(L"0FEE66C9-12BD-4278-8AAF-10C4E4068220", L"C", L"C:\\");
	bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksToolbarFolder(), std::move(bookmark),
		0);

	bookmark = std::make_unique<BookmarkItem>(L"08232430-DAF7-45DA-91FC-A350459C08CA",
		L"Public desktop", L"C:\\Users\\Public\\Desktop");
	bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksToolbarFolder(), std::move(bookmark),
		1);

	folder = std::make_unique<BookmarkItem>(L"DB3C84AB-5DE8-4820-B770-45F6C3C275EE",
		L"Other Folder 1", std::nullopt);
	rawFolder = bookmarkTree->AddBookmarkItem(bookmarkTree->GetOtherBookmarksFolder(),
		std::move(folder), 0);

	bookmark = std::make_unique<BookmarkItem>(L"B0AB36D9-E6B0-4D39-B24B-72BEB13F1D78", L"Sys32",
		L"C:\\Windows\\System32");
	bookmarkTree->AddBookmarkItem(rawFolder, std::move(bookmark), 0);
}

void BuildV1BasicLoadReferenceTree(BookmarkTree *bookmarkTree)
{
	auto bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Root", L"C:\\");
	bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksToolbarFolder(), std::move(bookmark),
		0);

	auto folder = std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt);
	auto *rawFolder = bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksToolbarFolder(),
		std::move(folder), 1);
	ASSERT_TRUE(rawFolder);

	bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Windows", L"C:\\Windows");
	bookmarkTree->AddBookmarkItem(rawFolder, std::move(bookmark), 0);

	folder = std::make_unique<BookmarkItem>(std::nullopt, L"Folder 2", std::nullopt);
	bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksMenuFolder(), std::move(folder), 0);
}

void BuildV1NestedShowOnToolbarLoadReferenceTree(BookmarkTree *bookmarkTree)
{
	auto bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Root", L"C:\\");
	bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksToolbarFolder(), std::move(bookmark),
		0);

	auto folder = std::make_unique<BookmarkItem>(std::nullopt, L"Folder 1", std::nullopt);
	auto *rawFolder =
		bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksMenuFolder(), std::move(folder), 0);
	ASSERT_TRUE(rawFolder);

	bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Windows", L"C:\\Windows");
	bookmarkTree->AddBookmarkItem(rawFolder, std::move(bookmark), 0);

	folder = std::make_unique<BookmarkItem>(std::nullopt, L"Folder 2", std::nullopt);
	bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksMenuFolder(), std::move(folder), 1);
}

void CompareBookmarkTrees(const BookmarkTree *firstTree, const BookmarkTree *secondTree,
	bool compareGuids)
{
	CompareFolders(firstTree->GetBookmarksMenuFolder(), secondTree->GetBookmarksMenuFolder(),
		compareGuids);
	CompareFolders(firstTree->GetBookmarksToolbarFolder(), secondTree->GetBookmarksToolbarFolder(),
		compareGuids);
	CompareFolders(firstTree->GetOtherBookmarksFolder(), secondTree->GetOtherBookmarksFolder(),
		compareGuids);
}

void CompareFolders(const BookmarkItem *firstFolder, const BookmarkItem *secondFolder,
	bool compareGuids)
{
	if (compareGuids)
	{
		EXPECT_EQ(firstFolder->GetGUID(), secondFolder->GetGUID());
	}

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
			CompareFolders(firstCurrent, secondCurrent, compareGuids);
		}
		else
		{
			CompareBookmarks(firstCurrent, secondCurrent, compareGuids);
		}
	}
}

void CompareBookmarks(const BookmarkItem *firstBookmark, const BookmarkItem *secondBookmark,
	bool compareGuids)
{
	if (compareGuids)
	{
		EXPECT_EQ(firstBookmark->GetGUID(), secondBookmark->GetGUID());
	}

	EXPECT_EQ(firstBookmark->GetName(), secondBookmark->GetName());
	EXPECT_EQ(firstBookmark->GetLocation(), secondBookmark->GetLocation());
}
