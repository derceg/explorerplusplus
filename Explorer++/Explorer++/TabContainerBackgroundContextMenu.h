// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include <boost/signals2.hpp>
#include <vector>

class BookmarkTree;
class BrowserWindow;
class CoreInterface;
class MenuView;
class ResourceLoader;
class TabContainerImpl;
class TabRestorer;

class TabContainerBackgroundContextMenu : public MenuBase
{
public:
	TabContainerBackgroundContextMenu(MenuView *menuView,
		const AcceleratorManager *acceleratorManager, TabContainerImpl *tabContainerImpl,
		TabRestorer *tabRestorer, BookmarkTree *bookmarkTree, BrowserWindow *browser,
		CoreInterface *coreInterface, const ResourceLoader *resourceLoader);

private:
	void BuildMenu();
	void OnMenuItemSelected(UINT menuItemId);

	TabContainerImpl *const m_tabContainerImpl;
	TabRestorer *const m_tabRestorer;
	BookmarkTree *const m_bookmarkTree;
	BrowserWindow *const m_browser;
	CoreInterface *const m_coreInterface;
	const ResourceLoader *const m_resourceLoader;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
