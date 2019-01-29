// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LuaPlugin.h"
#include "APIBinding.h"

Plugins::LuaPlugin::LuaPlugin(IExplorerplusplus *pexpp)
{
	BindAllApiMethods(m_lua, pexpp);
}

Plugins::LuaPlugin::~LuaPlugin()
{

}

sol::state &Plugins::LuaPlugin::GetLuaState()
{
	return m_lua;
}