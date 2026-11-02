// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserPaneView.h"

BrowserPaneView::BrowserPaneView(ViewBase *parent, MenuHelpTextHost *menuHelpTextHost,
	const DarkModeManager *darkModeManager, const DarkModeColorProvider *darkModeColorProvider,
	const Config *config, const ResourceLoader *resourceLoader) :
	m_tabBackingView(parent, menuHelpTextHost, darkModeManager, darkModeColorProvider, config),
	m_tabView(m_tabBackingView.GetHWND(), config, resourceLoader)
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
