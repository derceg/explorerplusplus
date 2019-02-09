// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LuaPlugin.h"
#include "APIBinding.h"

inline int onPanic(lua_State *L);

Plugins::LuaPlugin::LuaPlugin(const std::wstring &directory, const Manifest &manifest,
	TabContainerInterface *tabContainer, PluginMenuManager *pluginMenuManager) :
	m_directory(directory),
	m_manifest(manifest),
	m_lua(onPanic)
{
	BindAllApiMethods(m_lua, tabContainer, pluginMenuManager);
}

Plugins::LuaPlugin::~LuaPlugin()
{

}

std::wstring Plugins::LuaPlugin::GetDirectory()
{
	return m_directory;
}

Plugins::Manifest Plugins::LuaPlugin::GetManifest()
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