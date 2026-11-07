// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "TabBarBackingViewDelegate.h"

class AppServices;
class BrowserWindow;
class TabBar;
class TabBarBackingView;

class TabBarBacking : private TabBarBackingViewDelegate
{
public:
	TabBarBacking(TabBarBackingView *view, BrowserWindow *browser, TabBar *tabBar,
		AppServices *appServices);

private:
	// TabBackingViewDelegate
	void OnDoubleClicked() override;
	void OnShowContextMenu(const POINT &ptScreen) override;

	TabBarBackingView *const m_view;
	BrowserWindow *const m_browser;
	TabBar *const m_tabBar;
	AppServices *const m_appServices;
};
