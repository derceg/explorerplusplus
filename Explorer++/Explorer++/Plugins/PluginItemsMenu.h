// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"

namespace Plugins
{

class PluginMenuItemsRegistry;

// Represents a menu that contains items added by plugins.
class PluginItemsMenu : public MenuBase
{
public:
	PluginItemsMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		const PluginMenuItemsRegistry *pluginMenuItemsRegistry);

private:
	void BuildMenu(const PluginMenuItemsRegistry *pluginMenuItemsRegistry);
};

}
