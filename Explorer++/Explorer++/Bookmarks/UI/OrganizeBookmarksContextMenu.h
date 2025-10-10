// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include "../Helper/FileOperations.h"
#include <boost/signals2.hpp>
#include <vector>

class BookmarkItem;
class BookmarkTree;
class OrganizeBookmarksContextMenuDelegate;
class PlatformContext;
class ResourceLoader;

class OrganizeBookmarksContextMenu : public MenuBase
{
public:
	OrganizeBookmarksContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		HWND parentWindow, BookmarkTree *bookmarkTree, BookmarkItem *targetFolder,
		OrganizeBookmarksContextMenuDelegate *delegate, const ResourceLoader *resourceLoader,
		PlatformContext *platformContext);

private:
	void BuildMenu();

	void OnMenuItemSelected(UINT menuItemId);
	void OnNewBookmark();
	void OnNewFolder();
	void OnCopy(ClipboardAction action);
	void OnPaste();
	void OnDelete();
	void OnSelectAll();
	size_t GetTargetIndex() const;

	void OnBookmarkItemPreRemoval(BookmarkItem &bookmarkItem);

	const HWND m_parentWindow;
	BookmarkTree *const m_bookmarkTree;
	BookmarkItem *m_targetFolder = nullptr;
	OrganizeBookmarksContextMenuDelegate *const m_delegate;
	const ResourceLoader *const m_resourceLoader;
	PlatformContext *const m_platformContext;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
