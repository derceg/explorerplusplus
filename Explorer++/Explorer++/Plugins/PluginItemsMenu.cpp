// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Plugins/PluginItemsMenu.h"
#include "MenuView.h"
#include "Plugins/PluginMenuItemsRegistry.h"

namespace Plugins
{

PluginItemsMenu::PluginItemsMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
	const PluginMenuItemsRegistry *pluginMenuItemsRegistry) :
	MenuBase(menuView, acceleratorManager)
{
	BuildMenu(pluginMenuItemsRegistry);
}

void PluginItemsMenu::BuildMenu(const PluginMenuItemsRegistry *pluginMenuItemsRegistry)
{
	for (const auto *item : pluginMenuItemsRegistry->GetItems())
	{
		m_rootMenuView->AppendItem(
			MenuItemParams(item->text).SetSelectedCallback(item->selectedCallback));
	}
}

}
