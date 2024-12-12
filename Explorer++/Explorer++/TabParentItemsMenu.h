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
		BrowserWindow *browserWindow, ShellIconLoader *shellIconLoader,
		UINT startId = DEFAULT_START_ID, UINT endId = DEFAULT_END_ID);

private:
	static std::vector<PidlAbsolute> GetParentPidlCollection(const ShellBrowser *shellBrowser);
};
