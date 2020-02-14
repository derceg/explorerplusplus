// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class AcceleratorUpdater;
__interface IExplorerplusplus;
class Navigation;
class TabContainer;
class UiTheming;

namespace Plugins
{
	class PluginCommandManager;
	class PluginMenuManager;
}

__interface PluginInterface
{
	IExplorerplusplus *GetCoreInterface();
	TabContainer *GetTabContainer();
	Navigation *GetNavigation();
	Plugins::PluginMenuManager *GetPluginMenuManager();
	UiTheming *GetUiTheming();
	AcceleratorUpdater *GetAccleratorUpdater();
	Plugins::PluginCommandManager *GetPluginCommandManager();
};