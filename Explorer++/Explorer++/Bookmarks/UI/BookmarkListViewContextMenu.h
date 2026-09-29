// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkItem.h"
#include "MenuBase.h"
#include "../Helper/WeakPtr.h"

class BookmarkItemCreationDelegate;
class ResourceLoader;

class BookmarkListViewContextMenu : public MenuBase
{
public:
	enum class Item
	{
		NewBookmark,
		NewFolder
	};

	BookmarkListViewContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BookmarkItemCreationDelegate *delegate, WeakPtr<BookmarkItem> targetFolder,
		const ResourceLoader *resourceLoader);

private:
	void BuildMenu();
	void CreateBookmarkItem(BookmarkItem::Type type);

	BookmarkItemCreationDelegate *const m_delegate;
	WeakPtr<BookmarkItem> m_targetFolder;
	const ResourceLoader *const m_resourceLoader;
};
