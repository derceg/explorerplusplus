// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "TabBarBackingView.h"
#include "TabBarView.h"

struct Config;
class ResourceLoader;
class ThemeManager;
class ViewBase;

class BrowserPaneView
{
public:
	BrowserPaneView(ViewBase *parent, const ThemeManager *themeManager, const Config *config,
		const ResourceLoader *resourceLoader);

	TabBarBackingView *GetTabBarBackingView();
	TabBarView *GetTabBarView();

private:
	TabBarBackingView m_tabBarBackingView;
	TabBarView m_tabBarView;
};
