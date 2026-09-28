// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"

class BrowserWindow;
class ResourceLoader;
class ShellBrowser;

// Shows the list of available ViewMode values and allows the current ViewMode to be changed. This
// always operates on the active tab.
class ViewsMenu : public MenuBase
{
public:
	ViewsMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BrowserWindow *browser, const ResourceLoader *resourceLoader);

private:
	void BuildMenu(const ResourceLoader *resourceLoader);
	ShellBrowser *GetActiveShellBrowser() const;

	BrowserWindow *const m_browser;
};
