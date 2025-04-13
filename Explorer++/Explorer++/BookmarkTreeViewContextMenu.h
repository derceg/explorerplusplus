// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include <boost/signals2.hpp>
#include <vector>

class BookmarkItem;
class BookmarkTree;
class ResourceLoader;

class BookmarkTreeViewContextMenuDelegate
{
public:
	virtual ~BookmarkTreeViewContextMenuDelegate() = default;

	virtual void StartRenamingFolder(BookmarkItem *folder) = 0;
	virtual void CreateNewFolder(BookmarkItem *parentFolder) = 0;
};

class BookmarkTreeViewContextMenu : public MenuBase
{
public:
	BookmarkTreeViewContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BookmarkTreeViewContextMenuDelegate *delegate, BookmarkTree *bookmarkTree,
		BookmarkItem *targetItem, const ResourceLoader *resourceLoader);

private:
	void BuildMenu();

	void OnMenuItemSelected(UINT menuItemId);
	void DeleteItem();

	BookmarkTreeViewContextMenuDelegate *const m_delegate;
	BookmarkTree *const m_bookmarkTree;
	BookmarkItem *const m_targetFolder;
	const ResourceLoader *const m_resourceLoader;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
