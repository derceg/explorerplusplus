// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkItem.h"

class BookmarkTree;
__interface IExplorerplusplus;

class BookmarkContextMenuController
{
public:
	BookmarkContextMenuController(
		BookmarkTree *bookmarkTree, HMODULE resourceModule, IExplorerplusplus *expp);

	void OnMenuItemSelected(int menuItemId, BookmarkItem *targetParentFolder, size_t targetIndex,
		const RawBookmarkItems &bookmarkItems, HWND parentWindow);

private:
	void OnOpenAll(const RawBookmarkItems &bookmarkItems);
	void OnNewBookmarkItem(BookmarkItem::Type type, BookmarkItem *targetParentFolder,
		size_t targetIndex, HWND parentWindow);
	void OnCopy(const RawBookmarkItems &bookmarkItems, bool cut);
	void OnPaste(BookmarkItem *targetParentFolder, size_t targetIndex);
	void OnDelete(const RawBookmarkItems &bookmarkItems);
	void OnEditBookmarkItem(BookmarkItem *bookmarkItem, HWND parentWindow);

	BookmarkTree *m_bookmarkTree;
	HMODULE m_resourceModule;
	IExplorerplusplus *m_expp;
};