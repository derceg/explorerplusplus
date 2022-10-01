// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "Bookmarks/BookmarkDropper.h"
#include "BookmarkTreeHelper.h"
#include "Bookmarks/BookmarkDataExchange.h"
#include "Bookmarks/BookmarkTree.h"
#include "../Helper/DataExchangeHelper.h"
#include "../Helper/DataObjectImpl.h"
#include "../Helper/DragDropHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class BookmarkDropperValidTest : public Test
{
protected:
	BookmarkDropperValidTest()
	{
		auto grandparentFolder =
			std::make_unique<BookmarkItem>(std::nullopt, L"Test grandparent folder", std::nullopt);
		m_rawGrandparentFolder = grandparentFolder.get();
		m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
			std::move(grandparentFolder), 0);

		auto parentFolder =
			std::make_unique<BookmarkItem>(std::nullopt, L"Test parent folder", std::nullopt);
		m_rawParentFolder = parentFolder.get();
		m_bookmarkTree.AddBookmarkItem(m_rawGrandparentFolder, std::move(parentFolder), 0);

		auto bookmark = std::make_unique<BookmarkItem>(std::nullopt, L"Test bookmark", L"C:\\");
		m_rawBookmark = bookmark.get();
		m_bookmarkTree.AddBookmarkItem(m_rawParentFolder, std::move(bookmark), 0);

		auto secondTopLevelFolder =
			std::make_unique<BookmarkItem>(std::nullopt, L"Second top-level folder", std::nullopt);
		m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksMenuFolder(),
			std::move(secondTopLevelFolder), 1);

		auto &ownedPtr =
			m_rawGrandparentFolder->GetParent()->GetChildOwnedPtr(m_rawGrandparentFolder);
		m_dataObject = BookmarkDataExchange::CreateDataObject({ ownedPtr });

		m_dropper = std::make_unique<BookmarkDropper>(m_dataObject.get(), &m_bookmarkTree);
	}

	BookmarkTree m_bookmarkTree;

	std::unique_ptr<BookmarkDropper> m_dropper;
	winrt::com_ptr<IDataObject> m_dataObject;
	BookmarkItem *m_rawGrandparentFolder;
	BookmarkItem *m_rawParentFolder;
	BookmarkItem *m_rawBookmark;
};

class BookmarkDropperInvalidTest : public Test
{
protected:
	BookmarkDropperInvalidTest()
	{
		FORMATETC formatEtc = { CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

		auto global = WriteStringToGlobal(L"Test");
		STGMEDIUM stgMedium = GetStgMediumForGlobal(global.get());

		m_dataObject = winrt::make_self<DataObjectImpl>(&formatEtc, &stgMedium, 1);

		global.release();

		m_dropper = std::make_unique<BookmarkDropper>(m_dataObject.get(), &m_bookmarkTree);
	}

	BookmarkTree m_bookmarkTree;

	std::unique_ptr<BookmarkDropper> m_dropper;
	winrt::com_ptr<IDataObject> m_dataObject;
};

TEST_F(BookmarkDropperValidTest, DropEffect)
{
	// Drops on the root folder should be blocked.
	DWORD effect = m_dropper->GetDropEffect(m_bookmarkTree.GetRoot(), 0);
	EXPECT_EQ(effect, DROPEFFECT_NONE);

	// An item can't be dropped at its current position.
	effect = m_dropper->GetDropEffect(m_bookmarkTree.GetBookmarksMenuFolder(), 0);
	EXPECT_EQ(effect, DROPEFFECT_NONE);

	// It also can't be dropped after itself (which would really just be the same as being dropped
	// at its current position).
	effect = m_dropper->GetDropEffect(m_bookmarkTree.GetBookmarksMenuFolder(), 1);
	EXPECT_EQ(effect, DROPEFFECT_NONE);

	// It should be possible to move the item to another position, though (such as after the item
	// that follows it).
	effect = m_dropper->GetDropEffect(m_bookmarkTree.GetBookmarksMenuFolder(), 2);
	EXPECT_EQ(effect, DROPEFFECT_MOVE);

	effect = m_dropper->GetDropEffect(m_bookmarkTree.GetBookmarksToolbarFolder(), 0);
	EXPECT_EQ(effect, DROPEFFECT_MOVE);

	effect = m_dropper->GetDropEffect(m_bookmarkTree.GetOtherBookmarksFolder(), 0);
	EXPECT_EQ(effect, DROPEFFECT_MOVE);

	// An item can't be dropped on itself.
	effect = m_dropper->GetDropEffect(m_rawGrandparentFolder, 0);
	EXPECT_EQ(effect, DROPEFFECT_NONE);

	// It also can't be dropped on one of its children.
	effect = m_dropper->GetDropEffect(m_rawParentFolder, 0);
	EXPECT_EQ(effect, DROPEFFECT_NONE);
}

TEST_F(BookmarkDropperValidTest, DropOnRoot)
{
	DWORD effect = m_dropper->PerformDrop(m_bookmarkTree.GetRoot(), 0);
	EXPECT_EQ(effect, DROPEFFECT_NONE);
	EXPECT_EQ(m_rawGrandparentFolder->GetParent(), m_bookmarkTree.GetBookmarksMenuFolder());
}

TEST_F(BookmarkDropperValidTest, DropOnFolder)
{
	BookmarkTreeObserver observer;

	m_bookmarkTree.bookmarkItemMovedSignal.AddObserver(
		std::bind_front(&BookmarkTreeObserver::OnBookmarkItemMoved, &observer));

	EXPECT_CALL(observer,
		OnBookmarkItemMoved(m_rawGrandparentFolder, m_bookmarkTree.GetBookmarksMenuFolder(), 0,
			m_bookmarkTree.GetBookmarksToolbarFolder(), 0));

	DWORD effect = m_dropper->PerformDrop(m_bookmarkTree.GetBookmarksToolbarFolder(), 0);
	EXPECT_EQ(effect, DROPEFFECT_MOVE);
}

TEST_F(BookmarkDropperInvalidTest, DropEffect)
{
	// There are no bookmarks contained within the drop, so it shouldn't be
	// possible to drop anywhere.
	DWORD effect = m_dropper->GetDropEffect(m_bookmarkTree.GetRoot(), 0);
	EXPECT_EQ(effect, DROPEFFECT_NONE);

	effect = m_dropper->GetDropEffect(m_bookmarkTree.GetBookmarksMenuFolder(), 0);
	EXPECT_EQ(effect, DROPEFFECT_NONE);

	effect = m_dropper->GetDropEffect(m_bookmarkTree.GetBookmarksToolbarFolder(), 0);
	EXPECT_EQ(effect, DROPEFFECT_NONE);

	effect = m_dropper->GetDropEffect(m_bookmarkTree.GetOtherBookmarksFolder(), 0);
	EXPECT_EQ(effect, DROPEFFECT_NONE);
}

TEST_F(BookmarkDropperInvalidTest, Drop)
{
	DWORD effect = m_dropper->PerformDrop(m_bookmarkTree.GetBookmarksMenuFolder(), 0);
	EXPECT_EQ(effect, DROPEFFECT_NONE);
}
