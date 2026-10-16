// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MainToolbar.h"
#include "MenuBase.h"
#include <memory>
#include <vector>

class BrowserWindow;
class ResourceLoader;

class MainToolbarOverflowMenu : public MenuBase
{
public:
	MainToolbarOverflowMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		const std::vector<MainToolbar::OverflowItem> &overflowItems, BrowserWindow *browser,
		const ResourceLoader *resourceLoader);

private:
	void BuildMenu(const std::vector<MainToolbar::OverflowItem> &overflowItems,
		const ResourceLoader *resourceLoader);
	void AddOverflowItem(const MainToolbar::OverflowItem &item,
		const ResourceLoader *resourceLoader);

	void SendCommandToBrowser(WORD id);

	BrowserWindow *const m_browser;
	std::vector<std::unique_ptr<MenuBase>> m_childMenus;
};
