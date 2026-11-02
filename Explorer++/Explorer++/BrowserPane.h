// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "TabBacking.h"
#include "TabContainer.h"

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

	TabContainer *GetTabContainer();

private:
	TabContainer m_tabContainer;
	TabBacking m_tabBacking;
};
