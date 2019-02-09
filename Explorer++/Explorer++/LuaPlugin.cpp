// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LuaPlugin.h"
#include "APIBinding.h"

Plugins::LuaPlugin::LuaPlugin(TabContainerInterface *tabContainer, PluginMenuManager *pluginMenuManager)
{
	BindAllApiMethods(m_lua, tabContainer, pluginMenuManager);
}

Plugins::LuaPlugin::~LuaPlugin()
{

}

sol::state &Plugins::LuaPlugin::GetLuaState()
{
	return m_lua;
}