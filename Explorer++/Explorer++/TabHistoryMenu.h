// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"

class BrowserWindow;
class HistoryEntry;
class ShellBrowser;
class ShellIconLoader;

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

	TabHistoryMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BrowserWindow *browserWindow, ShellIconLoader *shellIconLoader, MenuType type);

private:
	void Initialize();
	void BuildMenu();
	void AddMenuItemForHistoryEntry(const HistoryEntry *entry);

	void OnMenuItemSelected(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown);
	void OnMenuItemMiddleClicked(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown);
	void NavigateToHistoryEntry(UINT menuItemId, bool isMiddleButtonDown, bool isCtrlKeyDown,
		bool isShiftKeyDown);
	ShellBrowser *GetShellBrowser() const;

	BrowserWindow *m_browserWindow = nullptr;
	ShellIconLoader *const m_shellIconLoader;
	const MenuType m_type;
	UINT m_idCounter = 1;

	std::vector<boost::signals2::scoped_connection> m_connections;
};
