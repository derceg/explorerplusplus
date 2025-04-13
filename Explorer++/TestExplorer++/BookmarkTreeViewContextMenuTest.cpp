// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "BookmarkTreeViewContextMenu.h"
#include "AcceleratorManager.h"
#include "Bookmarks/BookmarkTree.h"
#include "MainResource.h"
#include "PopupMenuView.h"
#include "Win32ResourceLoader.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace
{

class BookmarkTreeViewContextMenuDelegateMock : public BookmarkTreeViewContextMenuDelegate
{
public:
	MOCK_METHOD(void, StartRenamingFolder, (BookmarkItem * folder), (override));
	MOCK_METHOD(void, CreateNewFolder, (BookmarkItem * parentFolder), (override));
};

}

TEST(BookmarkTreeViewContextMenuTest, Selection)
{
	AcceleratorManager acceleratorManager;
	BookmarkTreeViewContextMenuDelegateMock delegate;

	BookmarkTree bookmarkTree;
	auto *targetFolder = bookmarkTree.AddBookmarkItem(bookmarkTree.GetBookmarksToolbarFolder(),
		std::make_unique<BookmarkItem>(std::nullopt, L"Target folder", std::nullopt), 0);

	Win32ResourceLoader resourceLoader(GetModuleHandle(nullptr));

	PopupMenuView popupMenu;
	BookmarkTreeViewContextMenu contextMenu(&popupMenu, &acceleratorManager, &delegate,
		&bookmarkTree, targetFolder, &resourceLoader);

	EXPECT_CALL(delegate, StartRenamingFolder(targetFolder));
	popupMenu.SelectItem(IDM_BOOKMARK_TREEVIEW_CONTEXT_MENU_RENAME, false, false);

	EXPECT_CALL(delegate, CreateNewFolder(targetFolder));
	popupMenu.SelectItem(IDM_BOOKMARK_TREEVIEW_CONTEXT_MENU_NEW_FOLDER, false, false);

	MockFunction<void(const std::wstring &guid)> removedCallback;
	bookmarkTree.bookmarkItemRemovedSignal.AddObserver(removedCallback.AsStdFunction());

	EXPECT_CALL(removedCallback, Call(targetFolder->GetGUID()));
	popupMenu.SelectItem(IDM_BOOKMARK_TREEVIEW_CONTEXT_MENU_DELETE, false, false);
}
