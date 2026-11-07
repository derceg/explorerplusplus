// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabBarBacking.h"
#include "AppServices.h"
#include "BrowserWindow.h"
#include "PopupMenuView.h"
#include "TabBar.h"
#include "TabBarBackgroundContextMenu.h"
#include "TabBarBackingView.h"

TabBarBacking::TabBarBacking(TabBarBackingView *view, BrowserWindow *browser, TabBar *tabBar,
	AppServices *appServices) :
	m_view(view),
	m_browser(browser),
	m_tabBar(tabBar),
	m_appServices(appServices)
{
	m_view->SetDelegate(this);
}

void TabBarBacking::OnDoubleClicked()
{
	m_tabBar->CreateNewTabInDefaultDirectory({ .selected = true });
}

void TabBarBacking::OnShowContextMenu(const POINT &ptScreen)
{
	PopupMenuView popupMenu(m_view->GetHWND(), m_browser);
	TabBarBackgroundContextMenu menu(&popupMenu, m_appServices->GetAcceleratorManager(), m_tabBar,
		m_appServices->GetTabRestorer(), m_appServices->GetBookmarkTree(), m_browser,
		m_appServices->GetResourceLoader(), m_appServices->GetPlatformContext());
	popupMenu.Show(ptScreen);
}
