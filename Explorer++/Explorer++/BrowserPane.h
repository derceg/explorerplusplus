// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MainTabView.h"
#include "TabContainer.h"

class AppServices;
class BrowserWindow;
class ShellBrowserFactory;

// Each browser pane contains a set of tabs, with each tab showing a file listing.
class BrowserPane
{
public:
	BrowserPane(BrowserWindow *browser, HWND tabViewParent,
		ShellBrowserFactory *shellBrowserFactory, AppServices *appServices);

	TabContainer *GetTabContainer();

private:
	MainTabView m_tabView;
	TabContainer m_tabContainer;
};
