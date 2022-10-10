// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class AcceleratorUpdater;
class CoreInterface;
class TabContainer;
class UiTheming;

namespace Plugins
{
class PluginCommandManager;
class PluginMenuManager;
}

__interface PluginInterface
{
	CoreInterface *GetCoreInterface();
	TabContainer *GetTabContainer();
	Plugins::PluginMenuManager *GetPluginMenuManager();
	UiTheming *GetUiTheming();
	AcceleratorUpdater *GetAccleratorUpdater();
	Plugins::PluginCommandManager *GetPluginCommandManager();
};
