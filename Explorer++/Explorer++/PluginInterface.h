// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "AcceleratorUpdater.h"
#include "CoreInterface.h"
#include "Navigation.h"
#include "PluginCommandManager.h"
#include "PluginMenuManager.h"
#include "TabContainer.h"
#include "UiTheming.h"

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