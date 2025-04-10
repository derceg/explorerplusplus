// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Plugins/LuaPlugin.h"
#include "Plugins/ApiBinding.h"
#include <sol/sol.hpp>

int Plugins::LuaPlugin::idCounter = 1;

inline int onPanic(lua_State *L);

Plugins::LuaPlugin::LuaPlugin(const std::wstring &directory, const Manifest &manifest,
	PluginInterface *pluginInterface, const Config *config) :
	m_directory(directory),
	m_manifest(manifest),
	m_lua(onPanic),
	m_id(idCounter++)
{
	BindAllApiMethods(m_id, m_lua, pluginInterface, config);
}

int Plugins::LuaPlugin::GetId() const
{
	return m_id;
}

std::wstring Plugins::LuaPlugin::GetDirectory() const
{
	return m_directory;
}

Plugins::Manifest Plugins::LuaPlugin::GetManifest() const
{
	return m_manifest;
}

sol::state &Plugins::LuaPlugin::GetLuaState()
{
	return m_lua;
}

inline int onPanic(lua_State *L)
{
	UNREFERENCED_PARAMETER(L);

	throw Plugins::LuaPanicException("A Lua panic occurred.");
}
