// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include <wil/com.h>
#include <commctrl.h>
#include <commoncontrols.h>

class BrowserWindow;
class HistoryEntry;
class ShellBrowser;

// Displays a set of history entries for the current tab, with the name and icon of each entry being
// displayed. An entry can be both clicked (which will cause the current tab to navigate
// back/forward to that entry) and middle-clicked (which will open the pidl associated with that
// entry in a new tab), with the ctrl and shift keys used to control exactly how an entry is opened.
class TabHistoryMenu : public MenuBase
{
public:
	enum class MenuType
	{
		Back,
		Forward
	};

	TabHistoryMenu(MenuView *menuView, BrowserWindow *browserWindow, MenuType type);

private:
	void Initialize();
	void BuildMenu();
	void AddMenuItemForHistoryEntry(HistoryEntry *entry);

	void OnMenuItemSelected(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown);
	void OnMenuItemMiddleClicked(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown);
	void NavigateToHistoryEntry(UINT menuItemId, bool isMiddleButtonDown, bool isCtrlKeyDown,
		bool isShiftKeyDown);
	ShellBrowser *GetShellBrowser() const;

	BrowserWindow *m_browserWindow = nullptr;
	const MenuType m_type;
	wil::com_ptr_nothrow<IImageList> m_systemImageList;
	int m_defaultFolderIconIndex;
	UINT m_idCounter = 1;

	std::vector<boost::signals2::scoped_connection> m_connections;
};
