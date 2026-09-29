// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkItem.h"
#include "MenuBase.h"
#include "../Helper/FileOperations.h"

class BookmarkTree;
class BrowserWindow;
class PlatformContext;
class ResourceLoader;

// Displays a context menu for one or more bookmarks. If multiple bookmarks are provided, they
// should all reside in the same parent folder.
class BookmarkContextMenu : public MenuBase
{
public:
	enum class Item
	{
		Open,
		OpenInNewTab,
		OpenAll,
		NewBookmark,
		NewFolder,
		Cut,
		Copy,
		Paste,
		Delete,
		Properties
	};

	BookmarkContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BookmarkTree *bookmarkTree, const RawBookmarkItems &bookmarkItems,
		const ResourceLoader *resourceLoader, BrowserWindow *browser, HWND parentWindow,
		PlatformContext *platformContext);

private:
	struct CreationTarget
	{
		BookmarkItem *folder = nullptr;
		size_t index = 0;
	};

	void BuildMenu();
	bool AreBookmarkItemsValid() const;
	size_t GetTotalBookmarks() const;
	bool HasPermanentNode() const;
	bool CanPaste() const;

	void OnOpen();
	void OnOpenInNewTab();
	void OnOpenAll();
	void OnNewBookmarkItem(BookmarkItem::Type type);
	void OnCopy(ClipboardAction action);
	void OnPaste();
	void OnDelete();
	void OnShowProperties();
	CreationTarget GetCreationTarget() const;

	BookmarkTree *const m_bookmarkTree;
	const RawBookmarkItems m_bookmarkItems;
	const ResourceLoader *const m_resourceLoader;
	BrowserWindow *const m_browser;
	const HWND m_parentWindow;
	PlatformContext *const m_platformContext;
};
