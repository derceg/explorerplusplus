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
		BrowserWindow *browserWindow, IconFetcher *iconFetcher, UINT menuStartId = 1,
		UINT menuEndId = (std::numeric_limits<UINT>::max)());
	~ShellItemsMenu();

	void RebuildMenu(const std::vector<PidlAbsolute> &pidls);

private:
	void AddMenuItemForPidl(PCIDLIST_ABSOLUTE pidl);
	wil::unique_hbitmap GetShellItemIcon(PCIDLIST_ABSOLUTE pidl);
	void QueueIconUpdateTask(PCIDLIST_ABSOLUTE pidl, UINT menuItemId);
	void OnIconRetrieved(UINT menuItemId, int iconIndex, int callbackId);

	void OnMenuItemSelected(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown);
	void OnMenuItemMiddleClicked(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown);
	void OpenSelectedItem(UINT menuItemId, bool isMiddleButtonDown, bool isCtrlKeyDown,
		bool isShiftKeyDown);

	BrowserWindow *const m_browserWindow;
	IconFetcher *const m_iconFetcher;
	const UINT m_menuStartId;
	const UINT m_menuEndId;
	UINT m_idCounter;
	wil::com_ptr_nothrow<IImageList> m_systemImageList;
	std::unordered_map<UINT, PidlAbsolute> m_idPidlMap;

	int m_iconCallbackIdCounter = 0;
	std::set<int> m_pendingIconCallbackIds;

	std::shared_ptr<bool> m_destroyed;

	std::vector<boost::signals2::scoped_connection> m_connections;
};
