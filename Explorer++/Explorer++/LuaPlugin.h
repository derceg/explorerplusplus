// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "PluginMenuManager.h"
#include "TabContainerInterface.h"
#include "../ThirdParty/Sol/sol.hpp"

namespace Plugins
{
	// Wraps a Lua state object and binds in all plugin API methods
	// during construction.
	class LuaPlugin
	{
	public:

		LuaPlugin(TabContainerInterface *tabContainer, PluginMenuManager *pluginMenuManager);
		~LuaPlugin();

		sol::state &GetLuaState();

	private:

		sol::state m_lua;
	};
}