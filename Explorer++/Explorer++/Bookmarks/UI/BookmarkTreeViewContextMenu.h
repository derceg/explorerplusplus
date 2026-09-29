// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include "../Helper/WeakPtr.h"

class BookmarkItem;
class BookmarkTree;
class ResourceLoader;

class BookmarkTreeViewContextMenuDelegate
{
public:
	virtual ~BookmarkTreeViewContextMenuDelegate() = default;

	virtual void StartRenamingFolder(BookmarkItem *folder) = 0;
	virtual void CreateFolder(BookmarkItem *parentFolder, size_t index) = 0;
};

class BookmarkTreeViewContextMenu : public MenuBase
{
public:
	enum class Item
	{
		Rename,
		Delete,
		NewFolder
	};

	BookmarkTreeViewContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BookmarkTreeViewContextMenuDelegate *delegate, BookmarkTree *bookmarkTree,
		WeakPtr<BookmarkItem> targetFolder, const ResourceLoader *resourceLoader);

private:
	void BuildMenu();

	void OnRename();
	void OnDelete();
	void OnNewFolder();

	BookmarkTreeViewContextMenuDelegate *const m_delegate;
	BookmarkTree *const m_bookmarkTree;
	WeakPtr<BookmarkItem> m_targetFolder;
	const ResourceLoader *const m_resourceLoader;
};
