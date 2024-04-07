// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "PopupMenuView.h"
#include <wil/com.h>
#include <commctrl.h>
#include <commoncontrols.h>
#include <memory>

class BrowserWindow;
class HistoryEntry;
class ShellBrowser;

// Displays a set of history entries for the current tab, with the name and icon of each entry being
// displayed. An entry can be both clicked (which will cause the current tab to navigate
// back/forward to that entry) and middle-clicked (which will open the pidl associated with that
// entry in a new tab), with the ctrl and shift keys used to control exactly how an entry is opened.
class TabHistoryMenu : public MenuController
{
public:
	enum class MenuType
	{
		Back,
		Forward
	};

	TabHistoryMenu(BrowserWindow *browserWindow, MenuType type);

	// This constructor is only used in tests. Although not ideal, it means that BrowserWindow and a
	// series of other classes don't have to be mocked, when the intent is to provide a specific
	// ShellBrowserInterface instance.
	TabHistoryMenu(ShellBrowser *shellBrowser, MenuType type);

	void Show(HWND hwnd, const POINT &point);

	// MenuController
	void OnMenuItemSelected(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown) override;
	void OnMenuItemMiddleClicked(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown) override;

	const PopupMenuView *GetMenuViewForTesting() const;

private:
	void Initialize();
	std::unique_ptr<PopupMenuView> BuildMenu();
	void AddMenuItemForHistoryEntry(PopupMenuView *menuView, HistoryEntry *entry);

	void NavigateToHistoryEntry(UINT menuItemId, bool isMiddleButtonDown, bool isCtrlKeyDown,
		bool isShiftKeyDown);
	ShellBrowser *GetShellBrowser() const;

	BrowserWindow *m_browserWindow = nullptr;

	// Only used in tests.
	ShellBrowser *m_testShellBrowser = nullptr;

	const MenuType m_type;
	std::unique_ptr<PopupMenuView> m_menuView;
	wil::com_ptr_nothrow<IImageList> m_systemImageList;
	int m_defaultFolderIconIndex;
	UINT m_idCounter = 1;
};
