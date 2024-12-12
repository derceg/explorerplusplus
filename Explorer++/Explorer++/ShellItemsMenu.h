// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include "../Helper/PidlHelper.h"

class BrowserWindow;
class ShellIconLoader;

// Displays a set of shell items in a menu, with the name and icon of each item being displayed.
// An item can be both clicked (to open it in the current tab) and middle-clicked (to open it in a
// new tab), with the ctrl and shift keys used to control exactly how an item is opened.
class ShellItemsMenu : public MenuBase
{
public:
	ShellItemsMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		const std::vector<PidlAbsolute> &pidls, BrowserWindow *browserWindow,
		ShellIconLoader *shellIconLoader, UINT startId = DEFAULT_START_ID,
		UINT endId = DEFAULT_END_ID);

	void RebuildMenu(const std::vector<PidlAbsolute> &pidls);

private:
	void AddMenuItemForPidl(PCIDLIST_ABSOLUTE pidl);

	void OnMenuItemSelected(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown);
	void OnMenuItemMiddleClicked(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown);
	void OpenSelectedItem(UINT menuItemId, bool isMiddleButtonDown, bool isCtrlKeyDown,
		bool isShiftKeyDown);

	BrowserWindow *const m_browserWindow;
	ShellIconLoader *const m_shellIconLoader;
	UINT m_idCounter;
	std::unordered_map<UINT, PidlAbsolute> m_idPidlMap;

	std::vector<boost::signals2::scoped_connection> m_connections;
};
