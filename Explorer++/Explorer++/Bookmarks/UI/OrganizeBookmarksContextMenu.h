// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include "../Helper/FileOperations.h"
#include "../Helper/WeakPtr.h"
#include <boost/signals2.hpp>
#include <vector>

class BookmarkItem;
class BookmarkTree;
class ClipboardStore;
class OrganizeBookmarksContextMenuDelegate;
class ResourceLoader;

class OrganizeBookmarksContextMenu : public MenuBase
{
public:
	OrganizeBookmarksContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BookmarkTree *bookmarkTree, WeakPtr<BookmarkItem> targetFolder,
		OrganizeBookmarksContextMenuDelegate *delegate, ClipboardStore *clipboardStore,
		const ResourceLoader *resourceLoader);

private:
	void BuildMenu();

	void OnMenuItemSelected(UINT menuItemId);
	void OnCopy(ClipboardAction action);
	void OnPaste();
	void OnDelete();
	void OnSelectAll();
	size_t GetTargetIndex() const;

	BookmarkTree *const m_bookmarkTree;
	WeakPtr<BookmarkItem> m_targetFolder;
	OrganizeBookmarksContextMenuDelegate *const m_delegate;
	ClipboardStore *const m_clipboardStore;
	const ResourceLoader *const m_resourceLoader;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
