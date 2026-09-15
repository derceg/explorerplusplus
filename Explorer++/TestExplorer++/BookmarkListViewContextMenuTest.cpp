// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/UI/BookmarkListViewContextMenu.h"
#include "AcceleratorManager.h"
#include "Bookmarks/BookmarkTree.h"
#include "Bookmarks/UI/BookmarkItemCreationDelegate.h"
#include "MainResource.h"
#include "MenuTestHost.h"
#include "ResourceLoaderFake.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace
{

class BookmarkItemCreationDelegateMock : public BookmarkItemCreationDelegate
{
public:
	MOCK_METHOD(void, CreateBookmark, (BookmarkItem * parentFolder, size_t index), (override));
	MOCK_METHOD(void, CreateFolder, (BookmarkItem * parentFolder, size_t index), (override));
};

}

class BookmarkListViewContextMenuTest : public Test
{
protected:
	BookmarkListViewContextMenuTest() :
		m_targetFolder(m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
			std::make_unique<BookmarkItem>(std::nullopt, L"Target folder", std::nullopt))),
		m_contextMenu(m_menuHost.GetView(), &m_acceleratorManager, &m_delegate,
			m_targetFolder->GetWeakPtr(), &m_resourceLoader)
	{
		m_bookmarkTree.AddBookmarkItem(m_targetFolder,
			std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 1", L"C:\\"));
		m_bookmarkTree.AddBookmarkItem(m_targetFolder,
			std::make_unique<BookmarkItem>(std::nullopt, L"Bookmark 2", L"D:\\"));
	}

	BookmarkTree m_bookmarkTree;
	BookmarkItem *m_targetFolder = nullptr;
	AcceleratorManager m_acceleratorManager;
	ResourceLoaderFake m_resourceLoader;

	BookmarkItemCreationDelegateMock m_delegate;

	MenuTestHost m_menuHost;
	BookmarkListViewContextMenu m_contextMenu;
};

TEST_F(BookmarkListViewContextMenuTest, Selection)
{
	// The target folder has 2 bookmarks, so the new bookmark should be added after those.
	EXPECT_CALL(m_delegate, CreateBookmark(m_targetFolder, 2));
	m_menuHost.SelectItem(IDM_BOOKMARK_LISTVIEW_CONTEXT_MENU_NEW_BOOKMARK, false, false);

	EXPECT_CALL(m_delegate, CreateFolder(m_targetFolder, 2));
	m_menuHost.SelectItem(IDM_BOOKMARK_LISTVIEW_CONTEXT_MENU_NEW_FOLDER, false, false);
}

TEST_F(BookmarkListViewContextMenuTest, SelectionAfterTargetFolderDestroyed)
{
	m_bookmarkTree.RemoveBookmarkItem(m_targetFolder);

	EXPECT_CALL(m_delegate, CreateBookmark(_, _)).Times(0);
	m_menuHost.SelectItem(IDM_BOOKMARK_LISTVIEW_CONTEXT_MENU_NEW_BOOKMARK, false, false);
}
