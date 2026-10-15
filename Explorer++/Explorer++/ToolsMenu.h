// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include <boost/signals2.hpp>
#include <memory>
#include <vector>

class BrowserWindow;
class FeatureList;
class ResourceLoader;

namespace Plugins
{

class PluginMenuItemsRegistry;

}

class ToolsMenu : public MenuBase
{
public:
	ToolsMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BrowserWindow *browser, const FeatureList *featureList,
		Plugins::PluginMenuItemsRegistry *pluginMenuItemsRegistry,
		const ResourceLoader *resourceLoader);

private:
	void RebuildMenu();
	void BuildMenu();

	void SendCommandToBrowser(WORD id);

	BrowserWindow *const m_browser;
	const FeatureList *const m_featureList;
	Plugins::PluginMenuItemsRegistry *const m_pluginMenuItemsRegistry;
	const ResourceLoader *const m_resourceLoader;
	std::unique_ptr<MenuBase> m_pluginsMenu;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
