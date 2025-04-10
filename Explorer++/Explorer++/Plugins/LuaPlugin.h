// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "PluginInterface.h"
#include "Plugins/Manifest.h"
#include <sol/forward.hpp>

struct Config;

namespace Plugins
{

// Wraps a Lua state object and binds in all plugin API methods during construction.
class LuaPlugin
{
public:
	LuaPlugin(const std::wstring &directory, const Manifest &manifest,
		PluginInterface *pluginInterface, const Config *config);

	int GetId() const;
	std::wstring GetDirectory() const;
	Plugins::Manifest GetManifest() const;
	sol::state &GetLuaState();

private:
	static int idCounter;

	std::wstring m_directory;
	Manifest m_manifest;

	sol::state m_lua;
	const int m_id;
};

class LuaPanicException : public std::runtime_error
{
public:
	LuaPanicException(const std::string &str) : std::runtime_error(str)
	{
	}
};

}
