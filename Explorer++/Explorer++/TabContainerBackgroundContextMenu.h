// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include <boost/signals2.hpp>
#include <vector>

class BookmarkTree;
class BrowserWindow;
class MenuView;
class PlatformContext;
class ResourceLoader;
class TabContainer;
class TabRestorer;

class TabContainerBackgroundContextMenu : public MenuBase
{
public:
	TabContainerBackgroundContextMenu(MenuView *menuView,
		const AcceleratorManager *acceleratorManager, TabContainer *tabContainer,
		TabRestorer *tabRestorer, BookmarkTree *bookmarkTree, BrowserWindow *browser,
		const ResourceLoader *resourceLoader, PlatformContext *platformContext);

private:
	void BuildMenu();
	void OnMenuItemSelected(UINT menuItemId);

	TabContainer *const m_tabContainer;
	TabRestorer *const m_tabRestorer;
	BookmarkTree *const m_bookmarkTree;
	BrowserWindow *const m_browser;
	const ResourceLoader *const m_resourceLoader;
	PlatformContext *const m_platformContext;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
