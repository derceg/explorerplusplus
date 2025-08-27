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

TEST(BookmarkTreeViewContextMenuTest, Selection)
{
	AcceleratorManager acceleratorManager;
	BookmarkTreeViewContextMenuDelegateMock delegate;

	BookmarkTree bookmarkTree;
	auto *targetFolder = bookmarkTree.AddBookmarkItem(bookmarkTree.GetBookmarksToolbarFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Target folder", std::nullopt), 0);

	ResourceLoaderFake resourceLoader;

	MenuViewFake menuView;
	BookmarkTreeViewContextMenu contextMenu(&menuView, &acceleratorManager, &delegate,
		&bookmarkTree, targetFolder, &resourceLoader);

	EXPECT_CALL(delegate, StartRenamingFolder(targetFolder));
	menuView.SelectItem(IDM_BOOKMARK_TREEVIEW_CONTEXT_MENU_RENAME, false, false);

	EXPECT_CALL(delegate, CreateFolder(targetFolder, targetFolder->GetChildren().size()));
	menuView.SelectItem(IDM_BOOKMARK_TREEVIEW_CONTEXT_MENU_NEW_FOLDER, false, false);

	MockFunction<void(const std::wstring &guid)> removedCallback;
	bookmarkTree.bookmarkItemRemovedSignal.AddObserver(removedCallback.AsStdFunction());

	EXPECT_CALL(removedCallback, Call(targetFolder->GetGUID()));
	menuView.SelectItem(IDM_BOOKMARK_TREEVIEW_CONTEXT_MENU_DELETE, false, false);
}
