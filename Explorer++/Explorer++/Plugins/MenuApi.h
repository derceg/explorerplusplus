// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <sol/forward.hpp>
#include <unordered_set>

namespace Plugins
{

class PluginMenuItemsRegistry;

class MenuApi
{
public:
	MenuApi(PluginMenuItemsRegistry *pluginMenuItemsRegistry);
	~MenuApi();

	UINT Create(const std::wstring &text, sol::protected_function callback);
	void Remove(UINT id);

private:
	PluginMenuItemsRegistry *const m_pluginMenuItemsRegistry;
	std::unordered_set<UINT> m_menuItemIds;
};

}
