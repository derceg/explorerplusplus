// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class AcceleratorUpdater;
class CoreInterface;
class TabContainer;
class TabEvents;
class UiTheming;

namespace Plugins
{

class PluginCommandManager;
class PluginMenuManager;

}

class PluginInterface
{
public:
	virtual ~PluginInterface() = default;

	virtual CoreInterface *GetCoreInterface() = 0;
	virtual TabEvents *GetTabEvents() = 0;
	virtual TabContainer *GetTabContainer() = 0;
	virtual Plugins::PluginMenuManager *GetPluginMenuManager() = 0;
	virtual UiTheming *GetUiTheming() = 0;
	virtual AcceleratorUpdater *GetAccleratorUpdater() = 0;
	virtual Plugins::PluginCommandManager *GetPluginCommandManager() = 0;
};
