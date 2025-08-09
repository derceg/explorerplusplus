// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkItem.h"
#include "MenuBase.h"
#include "../Helper/FileOperations.h"
#include <boost/signals2.hpp>
#include <vector>

class BookmarkTree;
class BrowserWindow;
class ClipboardStore;
class ResourceLoader;

// Displays a context menu for one or more bookmarks. If multiple bookmarks are provided, they
// should all reside in the same parent folder.
class BookmarkContextMenu : public MenuBase
{
public:
	BookmarkContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BookmarkTree *bookmarkTree, const RawBookmarkItems &bookmarkItems,
		const ResourceLoader *resourceLoader, BrowserWindow *browser, HWND parentWindow,
		ClipboardStore *clipboardStore);

private:
	void BuildMenu();
	bool AreBookmarkItemsValid();
	size_t GetTotalBookmarks();

	void OnMenuItemSelected(UINT menuItemId);
	void OnOpen();
	void OnOpenInNewTab();
	void OnOpenAll();
	void OnNewBookmarkItem(BookmarkItem::Type type, BookmarkItem *targetParentFolder,
		size_t targetIndex);
	void OnCopy(ClipboardAction action);
	void OnPaste(BookmarkItem *targetParentFolder, size_t targetIndex);
	void OnDelete();
	void OnShowProperties();

	BookmarkTree *const m_bookmarkTree;
	const RawBookmarkItems m_bookmarkItems;
	const ResourceLoader *const m_resourceLoader;
	BrowserWindow *const m_browser;
	const HWND m_parentWindow;
	ClipboardStore *const m_clipboardStore;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
