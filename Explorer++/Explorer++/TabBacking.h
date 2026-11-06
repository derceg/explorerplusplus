// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "TabBackingViewDelegate.h"

class AppServices;
class BrowserWindow;
class TabBackingView;
class TabContainer;

class TabBacking : private TabBackingViewDelegate
{
public:
	TabBacking(TabBackingView *view, BrowserWindow *browser, TabContainer *tabContainer,
		AppServices *appServices);

private:
	// TabBackingViewDelegate
	void OnDoubleClicked() override;
	void OnShowContextMenu(const POINT &ptScreen) override;

	TabBackingView *const m_view;
	BrowserWindow *const m_browser;
	TabContainer *const m_tabContainer;
	AppServices *const m_appServices;
};
