// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include <boost/signals2.hpp>
#include <vector>

class BrowserWindow;
struct Config;
class ResourceLoader;

class DisplayWindowContextMenu : public MenuBase
{
public:
	DisplayWindowContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BrowserWindow *browser, Config *config, const ResourceLoader *resourceLoader);

private:
	void BuildMenu();
	void OnMenuItemSelected(UINT menuItemId);

	BrowserWindow *const m_browser;
	Config *const m_config;
	const ResourceLoader *const m_resourceLoader;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
