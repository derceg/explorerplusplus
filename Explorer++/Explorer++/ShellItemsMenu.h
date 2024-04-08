// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include "../Helper/PidlHelper.h"
#include <wil/com.h>
#include <wil/resource.h>
#include <commoncontrols.h>

class BrowserWindow;
class IconFetcher;

// Displays a set of shell items in a menu, with the name and icon of each item being displayed.
// An item can be both clicked (to open it in the current tab) and middle-clicked (to open it in a
// new tab), with the ctrl and shift keys used to control exactly how an item is opened.
class ShellItemsMenu : public MenuBase
{
public:
	ShellItemsMenu(MenuView *menuView, const std::vector<PidlAbsolute> &pidls,
		BrowserWindow *browserWindow, IconFetcher *iconFetcher);
	~ShellItemsMenu();

private:
	void BuildMenu(const std::vector<PidlAbsolute> &pidls);

	void AddMenuItemForPidl(PCIDLIST_ABSOLUTE pidl);
	wil::unique_hbitmap GetShellItemIcon(PCIDLIST_ABSOLUTE pidl);
	void OnIconRetrieved(UINT menuItemId, int iconIndex);

	void OnMenuItemSelected(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown);
	void OnMenuItemMiddleClicked(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown);
	void OpenSelectedItem(UINT menuItemId, bool isMiddleButtonDown, bool isCtrlKeyDown,
		bool isShiftKeyDown);

	BrowserWindow *m_browserWindow = nullptr;
	IconFetcher *m_iconFetcher = nullptr;
	UINT m_idCounter = 1;
	wil::com_ptr_nothrow<IImageList> m_systemImageList;
	std::unordered_map<UINT, PidlAbsolute> m_idPidlMap;
	std::shared_ptr<bool> m_destroyed;

	std::vector<boost::signals2::scoped_connection> m_connections;
};
