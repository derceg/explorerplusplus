// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "PluginCommandManager.h"
#include "PluginMenuManager.h"
#include "TabContainerInterface.h"
#include "UiTheming.h"

__interface PluginInterface
{
	TabContainerInterface *GetTabContainer();
	Plugins::PluginMenuManager *GetPluginMenuManager();
	UiTheming *GetUiTheming();
	Plugins::PluginCommandManager *GetPluginCommandManager();
};