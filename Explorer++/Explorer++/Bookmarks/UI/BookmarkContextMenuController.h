// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkItem.h"

class BookmarkTree;
class BrowserWindow;
class CoreInterface;
class IconResourceLoader;

class BookmarkContextMenuController
{
public:
	BookmarkContextMenuController(BookmarkTree *bookmarkTree, HINSTANCE resourceInstance,
		BrowserWindow *browserWindow, CoreInterface *coreInterface,
		const IconResourceLoader *iconResourceLoader);

	void OnMenuItemSelected(UINT menuItemId, BookmarkItem *targetParentFolder, size_t targetIndex,
		const RawBookmarkItems &bookmarkItems, HWND parentWindow);

private:
	void OnOpenAll(const RawBookmarkItems &bookmarkItems);
	void OnNewBookmarkItem(BookmarkItem::Type type, BookmarkItem *targetParentFolder,
		size_t targetIndex, HWND parentWindow);
	void OnCopy(const RawBookmarkItems &bookmarkItems, bool cut);
	void OnPaste(BookmarkItem *targetParentFolder, size_t targetIndex);
	void OnDelete(const RawBookmarkItems &bookmarkItems);
	void OnEditBookmarkItem(BookmarkItem *bookmarkItem, HWND parentWindow);

	BookmarkTree *m_bookmarkTree = nullptr;
	HINSTANCE m_resourceInstance;
	BrowserWindow *m_browserWindow = nullptr;
	CoreInterface *m_coreInterface = nullptr;
	const IconResourceLoader *const m_iconResourceLoader;
};
