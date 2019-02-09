// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Manifest.h"
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

		LuaPlugin(const std::wstring &directory, const Manifest &manifest, TabContainerInterface *tabContainer, PluginMenuManager *pluginMenuManager);
		~LuaPlugin();

		std::wstring GetDirectory();
		Plugins::Manifest GetManifest();
		sol::state &GetLuaState();

	private:

		std::wstring m_directory;
		Manifest m_manifest;

		sol::state m_lua;
	};

	class LuaPanicException : public std::runtime_error
	{
	public:

		LuaPanicException(const std::string &str) :
			std::runtime_error(str)
		{

		}
	};
}