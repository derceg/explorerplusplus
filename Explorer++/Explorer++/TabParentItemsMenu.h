// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellItemsMenu.h"

class ShellBrowser;

// Displays the set of parent items for the current tab.
class TabParentItemsMenu : public ShellItemsMenu
{
public:
	TabParentItemsMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BrowserWindow *browserWindow, IconFetcher *iconFetcher);
	TabParentItemsMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BrowserWindow *browserWindow, IconFetcher *iconFetcher, UINT menuStartId, UINT menuEndId);

private:
	static std::vector<PidlAbsolute> GetParentPidlCollection(const ShellBrowser *shellBrowser);
};
