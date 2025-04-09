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
class ResourceLoader;
class TabContainerImpl;
class TabRestorer;
class ThemeManager;

class TabContainerBackgroundContextMenu : public MenuBase
{
public:
	TabContainerBackgroundContextMenu(MenuView *menuView,
		const AcceleratorManager *acceleratorManager, TabContainerImpl *tabContainerImpl,
		TabRestorer *tabRestorer, BookmarkTree *bookmarkTree, CoreInterface *coreInterface,
		const ResourceLoader *resourceLoader, const IconResourceLoader *iconResourceLoader,
		ThemeManager *themeManager);

private:
	void BuildMenu();
	void OnMenuItemSelected(UINT menuItemId);

	TabContainerImpl *const m_tabContainerImpl;
	TabRestorer *const m_tabRestorer;
	BookmarkTree *const m_bookmarkTree;
	CoreInterface *const m_coreInterface;
	const ResourceLoader *const m_resourceLoader;
	const IconResourceLoader *const m_iconResourceLoader;
	ThemeManager *const m_themeManager;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
