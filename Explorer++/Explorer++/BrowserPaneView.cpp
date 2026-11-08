// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserPaneView.h"

BrowserPaneView::BrowserPaneView(ViewBase *parent, const ThemeManager *themeManager,
	const Config *config, const ResourceLoader *resourceLoader) :
	m_tabBarBackingView(parent, themeManager),
	m_tabBarView(&m_tabBarBackingView, themeManager, config, resourceLoader)
{
}

TabBarBackingView *BrowserPaneView::GetTabBarBackingView()
{
	return &m_tabBarBackingView;
}

TabBarView *BrowserPaneView::GetTabBarView()
{
	return &m_tabBarView;
}
