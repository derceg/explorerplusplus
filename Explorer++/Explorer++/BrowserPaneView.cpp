// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserPaneView.h"

BrowserPaneView::BrowserPaneView(ViewBase *parent, const DarkModeManager *darkModeManager,
	const DarkModeColorProvider *darkModeColorProvider,
	const LightModeColorProvider *lightModeColorProvider, const Config *config,
	const ResourceLoader *resourceLoader) :
	m_tabBarBackingView(parent, darkModeManager, darkModeColorProvider),
	m_tabBarView(m_tabBarBackingView.GetHWND(), darkModeManager, darkModeColorProvider,
		lightModeColorProvider, config, resourceLoader)
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
