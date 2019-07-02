// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "TabContainer.h"
#include "UiTheming.h"

TabContainer *Explorerplusplus::GetTabContainer()
{
	return m_tabContainer;
}

TabInterface *Explorerplusplus::GetTabInterface()
{
	return this;
}

Navigation *Explorerplusplus::GetNavigation()
{
	return m_navigation;
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