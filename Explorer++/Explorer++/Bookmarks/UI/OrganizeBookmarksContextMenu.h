// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkItem.h"
#include "MenuBase.h"
#include "../Helper/FileOperations.h"
#include "../Helper/WeakPtr.h"

class BookmarkTree;
class ClipboardStore;
class OrganizeBookmarksContextMenuDelegate;
class ResourceLoader;

class OrganizeBookmarksContextMenu : public MenuBase
{
public:
	enum class Item
	{
		NewBookmark,
		NewFolder,
		Cut,
		Copy,
		Paste,
		Delete,
		SelectAll
	};

	OrganizeBookmarksContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BookmarkTree *bookmarkTree, WeakPtr<BookmarkItem> targetFolder,
		OrganizeBookmarksContextMenuDelegate *delegate, ClipboardStore *clipboardStore,
		const ResourceLoader *resourceLoader);

private:
	void BuildMenu();

	bool CanPaste() const;
	bool CanRemoveSelectedItems() const;
	bool CanSelectAll() const;

	void OnNewBookmarkItem(BookmarkItem::Type type);
	void OnCopy(ClipboardAction action);
	void OnPaste();
	void OnDelete();
	size_t GetTargetIndex() const;

	BookmarkTree *const m_bookmarkTree;
	WeakPtr<BookmarkItem> m_targetFolder;
	OrganizeBookmarksContextMenuDelegate *const m_delegate;
	ClipboardStore *const m_clipboardStore;
	const ResourceLoader *const m_resourceLoader;
};
