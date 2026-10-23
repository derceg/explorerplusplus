// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserPane.h"
#include "AppServices.h"

BrowserPane::BrowserPane(BrowserWindow *browser, HWND tabViewParent,
	ShellBrowserFactory *shellBrowserFactory, AppServices *appServices) :
	m_tabView(tabViewParent, appServices->GetConfig(), appServices->GetResourceLoader()),
	m_tabContainer(&m_tabView, browser, shellBrowserFactory, appServices)
{
}

TabContainer *BrowserPane::GetTabContainer()
{
	return &m_tabContainer;
}
