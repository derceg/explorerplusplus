// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include "../Helper/WeakPtr.h"
#include <boost/signals2.hpp>
#include <vector>

class BookmarkItem;
class BookmarkItemCreationDelegate;
class ResourceLoader;

class BookmarkListViewContextMenu : public MenuBase
{
public:
	BookmarkListViewContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BookmarkItemCreationDelegate *delegate, WeakPtr<BookmarkItem> targetFolder,
		const ResourceLoader *resourceLoader);

private:
	void BuildMenu();
	void OnMenuItemSelected(UINT menuItemId);

	BookmarkItemCreationDelegate *const m_delegate;
	WeakPtr<BookmarkItem> m_targetFolder;
	const ResourceLoader *const m_resourceLoader;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
