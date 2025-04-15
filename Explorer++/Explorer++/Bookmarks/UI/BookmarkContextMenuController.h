// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkItem.h"

class AcceleratorManager;
class BookmarkTree;
class BrowserWindow;
class CoreInterface;
class ResourceLoader;

class BookmarkContextMenuController
{
public:
	BookmarkContextMenuController(BookmarkTree *bookmarkTree, const ResourceLoader *resourceLoader,
		HINSTANCE resourceInstance, BrowserWindow *browserWindow, CoreInterface *coreInterface,
		const AcceleratorManager *acceleratorManager);

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

	BookmarkTree *const m_bookmarkTree;
	const ResourceLoader *const m_resourceLoader;
	HINSTANCE m_resourceInstance;
	BrowserWindow *const m_browserWindow;
	CoreInterface *const m_coreInterface;
	const AcceleratorManager *const m_acceleratorManager;
};
