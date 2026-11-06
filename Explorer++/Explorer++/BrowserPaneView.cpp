// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserPaneView.h"

BrowserPaneView::BrowserPaneView(ViewBase *parent, const DarkModeManager *darkModeManager,
	const DarkModeColorProvider *darkModeColorProvider,
	const LightModeColorProvider *lightModeColorProvider, const Config *config,
	const ResourceLoader *resourceLoader) :
	m_tabBackingView(parent, darkModeManager, darkModeColorProvider),
	m_tabView(m_tabBackingView.GetHWND(), darkModeManager, darkModeColorProvider,
		lightModeColorProvider, config, resourceLoader)
{
}

TabBackingView *BrowserPaneView::GetTabBackingView()
{
	return &m_tabBackingView;
}

MainTabView *BrowserPaneView::GetTabView()
{
	return &m_tabView;
}
