// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/UI/BookmarkColumnHelper.h"
#include "Bookmarks/BookmarkTree.h"
#include "Bookmarks/UI/BookmarkColumn.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <compare>
#include <vector>

using namespace testing;

class BookmarkColumnHelperTest : public Test
{
protected:
	BookmarkColumnHelperTest()
	{
		auto *targetFolder = m_bookmarkTree.GetOtherBookmarksFolder();
		m_bookmarkY = m_bookmarkTree.AddBookmarkItem(targetFolder,
			std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark Y", L"e:\\"));
		m_folderB = m_bookmarkTree.AddBookmarkItem(targetFolder,
			std::make_unique<BookmarkItem>(std::nullopt, L"Folder B", std::nullopt));
		m_bookmarkZ = m_bookmarkTree.AddBookmarkItem(targetFolder,
			std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark Z", L"c:\\"));
		m_bookmarkX = m_bookmarkTree.AddBookmarkItem(targetFolder,
			std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark X", L"d:\\"));
		m_folderA = m_bookmarkTree.AddBookmarkItem(targetFolder,
			std::make_unique<BookmarkItem>(std::nullopt, L"Folder A", std::nullopt));

		for (const auto &child : targetFolder->GetChildren())
		{
			m_items.push_back(child.get());
		}
	}

	void SortItemsByColumn(BookmarkColumn column)
	{
		std::ranges::sort(m_items,
			[column](const BookmarkItem *firstItem, const BookmarkItem *secondItem)
			{
				auto cmp = CompareBookmarksByColumn(column, firstItem, secondItem);
				return std::is_lt(cmp);
			});
	}

	FILETIME BuildFileTime(WORD year, WORD month, WORD day)
	{
		SYSTEMTIME systemTime = {};
		systemTime.wYear = year;
		systemTime.wMonth = month;
		systemTime.wDay = day;

		FILETIME fileTime;
		auto res = SystemTimeToFileTime(&systemTime, &fileTime);
		CHECK(res);

		return fileTime;
	}

	BookmarkTree m_bookmarkTree;
	BookmarkItem *m_folderA = nullptr;
	BookmarkItem *m_folderB = nullptr;
	BookmarkItem *m_bookmarkX = nullptr;
	BookmarkItem *m_bookmarkY = nullptr;
	BookmarkItem *m_bookmarkZ = nullptr;
	std::vector<BookmarkItem *> m_items;
};

TEST_F(BookmarkColumnHelperTest, SortByName)
{
	SortItemsByColumn(BookmarkColumn::Name);
	EXPECT_THAT(m_items, ElementsAre(m_folderA, m_folderB, m_bookmarkX, m_bookmarkY, m_bookmarkZ));
}

TEST_F(BookmarkColumnHelperTest, SortByLocation)
{
	SortItemsByColumn(BookmarkColumn::Location);
	EXPECT_THAT(m_items, ElementsAre(m_folderA, m_folderB, m_bookmarkZ, m_bookmarkX, m_bookmarkY));
}

TEST_F(BookmarkColumnHelperTest, SortByDateCreated)
{
	m_bookmarkZ->SetDateCreated(BuildFileTime(2025, 10, 1));
	m_folderA->SetDateCreated(BuildFileTime(2025, 10, 2));
	m_bookmarkY->SetDateCreated(BuildFileTime(2025, 10, 3));
	m_bookmarkX->SetDateCreated(BuildFileTime(2025, 10, 4));
	m_folderB->SetDateCreated(BuildFileTime(2025, 10, 5));

	SortItemsByColumn(BookmarkColumn::DateCreated);
	EXPECT_THAT(m_items, ElementsAre(m_folderA, m_folderB, m_bookmarkZ, m_bookmarkY, m_bookmarkX));
}

TEST_F(BookmarkColumnHelperTest, SortByDateModified)
{
	m_folderB->SetDateModified(BuildFileTime(2025, 10, 1));
	m_bookmarkY->SetDateModified(BuildFileTime(2025, 10, 2));
	m_bookmarkX->SetDateModified(BuildFileTime(2025, 10, 3));
	m_folderA->SetDateModified(BuildFileTime(2025, 10, 4));
	m_bookmarkZ->SetDateModified(BuildFileTime(2025, 10, 5));

	SortItemsByColumn(BookmarkColumn::DateModified);
	EXPECT_THAT(m_items, ElementsAre(m_folderB, m_folderA, m_bookmarkY, m_bookmarkX, m_bookmarkZ));
}
