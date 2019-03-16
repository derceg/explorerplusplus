// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "TabContainerInterface.h"
#include "UiTheming.h"

TabContainerInterface *Explorerplusplus::GetTabContainer()
{
	return this;
}

TabInterface *Explorerplusplus::GetTabInterface()
{
	return this;
}

Plugins::PluginMenuManager *Explorerplusplus::GetPluginMenuManager()
{
	return &m_pluginMenuManager;
}

UiTheming *Explorerplusplus::GetUiTheming()
{
	return m_uiTheming.get();
}

Plugins::PluginCommandManager *Explorerplusplus::GetPluginCommandManager()
{
	return &m_pluginCommandManager;
}