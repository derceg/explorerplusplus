// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include "MenuDelegate.h"
#include "ShellBrowser/ViewModes.h"
#include <unordered_map>

class BrowserWindow;
class ResourceLoader;
class ShellBrowser;

// Shows the list of available ViewMode values and allows the current ViewMode to be changed. This
// always operates on the active tab.
class ViewsMenu : public MenuBase, private MenuDelegate
{
public:
	ViewsMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BrowserWindow *browser, const ResourceLoader *resourceLoader);

private:
	void BuildMenu(const ResourceLoader *resourceLoader);

	// MenuDelegate
	void OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown) override;

	ShellBrowser *GetActiveShellBrowser();

	BrowserWindow *const m_browser;
	UINT m_idCounter = 1;
	std::unordered_map<UINT, ViewMode> m_idToViewModeMap;
};
