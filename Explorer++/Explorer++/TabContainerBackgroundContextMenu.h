// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include <boost/signals2.hpp>
#include <vector>

class BookmarkTree;
class BrowserWindow;
class ClipboardStore;
class MenuView;
class ResourceLoader;
class TabContainer;
class TabRestorer;

class TabContainerBackgroundContextMenu : public MenuBase
{
public:
	TabContainerBackgroundContextMenu(MenuView *menuView,
		const AcceleratorManager *acceleratorManager, TabContainer *tabContainer,
		TabRestorer *tabRestorer, BookmarkTree *bookmarkTree, BrowserWindow *browser,
		ClipboardStore *clipboardStore, const ResourceLoader *resourceLoader);

private:
	void BuildMenu();
	void OnMenuItemSelected(UINT menuItemId);

	TabContainer *const m_tabContainer;
	TabRestorer *const m_tabRestorer;
	BookmarkTree *const m_bookmarkTree;
	BrowserWindow *const m_browser;
	ClipboardStore *const m_clipboardStore;
	const ResourceLoader *const m_resourceLoader;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
