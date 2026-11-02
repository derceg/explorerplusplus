// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserPane.h"
#include "AppServices.h"
#include "BrowserPaneView.h"

BrowserPane::BrowserPane(BrowserPaneView *view, BrowserWindow *browser,
	ShellBrowserFactory *shellBrowserFactory, AppServices *appServices) :
	m_tabContainer(view->GetTabView(), browser, shellBrowserFactory, appServices),
	m_tabBacking(view->GetTabBackingView(), browser, &m_tabContainer, appServices)
{
}

TabContainer *BrowserPane::GetTabContainer()
{
	return &m_tabContainer;
}
