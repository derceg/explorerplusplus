// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "AcceleratorUpdater.h"
#include "Navigation.h"
#include "PluginCommandManager.h"
#include "PluginMenuManager.h"
#include "TabContainer.h"
#include "TabInterface.h"
#include "UiTheming.h"

__interface PluginInterface
{
	TabContainer *GetTabContainer();
	TabInterface *GetTabInterface();
	Navigation *GetNavigation();
	Plugins::PluginMenuManager *GetPluginMenuManager();
	UiTheming *GetUiTheming();
	AcceleratorUpdater *GetAccleratorUpdater();
	Plugins::PluginCommandManager *GetPluginCommandManager();
};