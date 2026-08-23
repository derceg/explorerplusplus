// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/UI/BookmarkTreeViewContextMenu.h"
#include "AcceleratorManager.h"
#include "Bookmarks/BookmarkTree.h"
#include "MainResource.h"
#include "MenuViewFake.h"
#include "ResourceLoaderFake.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace
{

class BookmarkTreeViewContextMenuDelegateMock : public BookmarkTreeViewContextMenuDelegate
{
public:
	MOCK_METHOD(void, StartRenamingFolder, (BookmarkItem * folder), (override));
	MOCK_METHOD(void, CreateFolder, (BookmarkItem * parentFolder, size_t index), (override));
};

}

class BookmarkTreeViewContextMenuTest : public Test
{
protected:
	BookmarkTreeViewContextMenuTest() :
		m_targetFolder(m_bookmarkTree.AddBookmarkItem(m_bookmarkTree.GetBookmarksToolbarFolder(),
			std::make_unique<BookmarkItem>(std::nullopt, L"Target folder", std::nullopt), 0)),
		m_contextMenu(&m_menuView, &m_acceleratorManager, &m_delegate, &m_bookmarkTree,
			m_targetFolder->GetWeakPtr(), &m_resourceLoader)
	{
	}

	BookmarkTree m_bookmarkTree;
	BookmarkItem *m_targetFolder = nullptr;
	AcceleratorManager m_acceleratorManager;
	ResourceLoaderFake m_resourceLoader;

	BookmarkTreeViewContextMenuDelegateMock m_delegate;

	MenuViewFake m_menuView;
	BookmarkTreeViewContextMenu m_contextMenu;
};

TEST_F(BookmarkTreeViewContextMenuTest, Selection)
{
	EXPECT_CALL(m_delegate, StartRenamingFolder(m_targetFolder));
	m_menuView.SelectItem(IDM_BOOKMARK_TREEVIEW_CONTEXT_MENU_RENAME, false, false);

	EXPECT_CALL(m_delegate, CreateFolder(m_targetFolder, m_targetFolder->GetChildren().size()));
	m_menuView.SelectItem(IDM_BOOKMARK_TREEVIEW_CONTEXT_MENU_NEW_FOLDER, false, false);

	MockFunction<void(const std::wstring &guid)> removedCallback;
	m_bookmarkTree.bookmarkItemRemovedSignal.AddObserver(removedCallback.AsStdFunction());

	EXPECT_CALL(removedCallback, Call(m_targetFolder->GetGUID()));
	m_menuView.SelectItem(IDM_BOOKMARK_TREEVIEW_CONTEXT_MENU_DELETE, false, false);
}

TEST_F(BookmarkTreeViewContextMenuTest, SelectionAfterTargetFolderDestroyed)
{
	m_bookmarkTree.RemoveBookmarkItem(m_targetFolder);

	EXPECT_CALL(m_delegate, StartRenamingFolder(_)).Times(0);
	m_menuView.SelectItem(IDM_BOOKMARK_TREEVIEW_CONTEXT_MENU_RENAME, false, false);
}
