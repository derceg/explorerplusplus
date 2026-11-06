// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabBacking.h"
#include "AppServices.h"
#include "BrowserWindow.h"
#include "PopupMenuView.h"
#include "TabBackingView.h"
#include "TabContainer.h"
#include "TabContainerBackgroundContextMenu.h"

TabBacking::TabBacking(TabBackingView *view, BrowserWindow *browser, TabContainer *tabContainer,
	AppServices *appServices) :
	m_view(view),
	m_browser(browser),
	m_tabContainer(tabContainer),
	m_appServices(appServices)
{
	m_view->SetDelegate(this);
}

void TabBacking::OnDoubleClicked()
{
	m_tabContainer->CreateNewTabInDefaultDirectory({ .selected = true });
}

void TabBacking::OnShowContextMenu(const POINT &ptScreen)
{
	PopupMenuView popupMenu(m_view->GetHWND(), m_browser);
	TabContainerBackgroundContextMenu menu(&popupMenu, m_appServices->GetAcceleratorManager(),
		m_tabContainer, m_appServices->GetTabRestorer(), m_appServices->GetBookmarkTree(),
		m_browser, m_appServices->GetResourceLoader(), m_appServices->GetPlatformContext());
	popupMenu.Show(ptScreen);
}
