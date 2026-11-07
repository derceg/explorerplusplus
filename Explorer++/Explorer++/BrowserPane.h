// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "TabBar.h"
#include "TabBarBacking.h"

class AppServices;
class BrowserPaneView;
class BrowserWindow;
class ShellBrowserFactory;

// Each browser pane contains a set of tabs, with each tab showing a file listing.
class BrowserPane
{
public:
	BrowserPane(BrowserPaneView *view, BrowserWindow *browser,
		ShellBrowserFactory *shellBrowserFactory, AppServices *appServices);

	TabBar *GetTabBar();

private:
	TabBar m_tabBar;
	TabBarBacking m_tabBarBacking;
};
