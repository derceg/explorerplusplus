// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include <boost/signals2.hpp>
#include <vector>

class BookmarkTree;
class CoreInterface;
class IconResourceLoader;
class MenuView;
class TabContainer;
class TabRestorer;

class TabContainerBackgroundContextMenu : public MenuBase
{
public:
	TabContainerBackgroundContextMenu(MenuView *menuView,
		const AcceleratorManager *acceleratorManager, TabContainer *tabContainer,
		TabRestorer *tabRestorer, BookmarkTree *bookmarkTree, CoreInterface *coreInterface,
		const IconResourceLoader *iconResourceLoader);

private:
	void BuildMenu();
	void OnMenuItemSelected(UINT menuItemId);

	TabContainer *const m_tabContainer;
	TabRestorer *const m_tabRestorer;
	BookmarkTree *const m_bookmarkTree;
	CoreInterface *const m_coreInterface;
	const IconResourceLoader *const m_iconResourceLoader;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
