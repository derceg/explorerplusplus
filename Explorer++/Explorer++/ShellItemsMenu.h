// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include <shtypes.h>

class BrowserWindow;
class PidlAbsolute;
class ShellIconLoader;

// Displays a set of shell items in a menu, with the name and icon of each item being displayed.
// An item can be both clicked (to open it in the current tab) and middle-clicked (to open it in a
// new tab), with the ctrl and shift keys used to control exactly how an item is opened.
class ShellItemsMenu : public MenuBase
{
public:
	ShellItemsMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		const std::vector<PidlAbsolute> &pidls, BrowserWindow *browserWindow,
		ShellIconLoader *shellIconLoader);

	void RebuildMenu(const std::vector<PidlAbsolute> &pidls);

private:
	void AddMenuItemForPidl(const PidlAbsolute &pidl);

	void OpenSelectedItem(PCIDLIST_ABSOLUTE pidl, bool isMiddleButtonDown, bool isCtrlKeyDown,
		bool isShiftKeyDown);

	BrowserWindow *const m_browserWindow;
	ShellIconLoader *const m_shellIconLoader;
};
