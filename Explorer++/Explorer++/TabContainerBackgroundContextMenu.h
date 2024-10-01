// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include <boost/signals2.hpp>
#include <vector>

class BookmarkTree;
class CoreInterface;
class MenuView;
class TabContainer;

class TabContainerBackgroundContextMenu : public MenuBase
{
public:
	TabContainerBackgroundContextMenu(MenuView *menuView,
		const AcceleratorManager *acceleratorManager, TabContainer *tabContainer,
		BookmarkTree *bookmarkTree, CoreInterface *coreInterface);

private:
	void BuildMenu();
	void OnMenuItemSelected(UINT menuItemId);

	TabContainer *const m_tabContainer;
	BookmarkTree *const m_bookmarkTree;
	CoreInterface *const m_coreInterface;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
