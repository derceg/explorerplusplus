// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserPane.h"
#include "AppServices.h"
#include "BrowserPaneView.h"

BrowserPane::BrowserPane(BrowserPaneView *view, BrowserWindow *browser,
	ShellBrowserFactory *shellBrowserFactory, AppServices *appServices) :
	m_tabBar(view->GetTabBarView(), browser, shellBrowserFactory, appServices),
	m_tabBarBacking(view->GetTabBarBackingView(), browser, &m_tabBar, appServices)
{
}

TabBar *BrowserPane::GetTabBar()
{
	return &m_tabBar;
}
