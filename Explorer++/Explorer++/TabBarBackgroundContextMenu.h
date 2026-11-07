// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"

class BookmarkTree;
class BrowserWindow;
class MenuView;
class PlatformContext;
class ResourceLoader;
class TabBar;
class TabRestorer;

class TabBarBackgroundContextMenu : public MenuBase
{
public:
	TabBarBackgroundContextMenu(MenuView *menuView,
		const AcceleratorManager *acceleratorManager, TabBar *tabBar, TabRestorer *tabRestorer,
		BookmarkTree *bookmarkTree, BrowserWindow *browser, const ResourceLoader *resourceLoader,
		PlatformContext *platformContext);

private:
	void BuildMenu();

	TabBar *const m_tabBar;
	TabRestorer *const m_tabRestorer;
	BookmarkTree *const m_bookmarkTree;
	BrowserWindow *const m_browser;
	const ResourceLoader *const m_resourceLoader;
	PlatformContext *const m_platformContext;
};
