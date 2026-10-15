// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Plugins/MenuApi.h"
#include "Plugins/PluginMenuItemsRegistry.h"
#include <sol/sol.hpp>

Plugins::MenuApi::MenuApi(PluginMenuItemsRegistry *pluginMenuItemsRegistry) :
	m_pluginMenuItemsRegistry(pluginMenuItemsRegistry)
{
}

Plugins::MenuApi::~MenuApi()
{
	for (UINT id : m_menuItemIds)
	{
		m_pluginMenuItemsRegistry->RemoveMenuItem(id);
	}
}

UINT Plugins::MenuApi::Create(const std::wstring &text, sol::protected_function callback)
{
	UINT id = m_pluginMenuItemsRegistry->AddMenuItem(text, callback);
	m_menuItemIds.insert(id);
	return id;
}

void Plugins::MenuApi::Remove(UINT id)
{
	if (!m_menuItemIds.contains(id))
	{
		return;
	}

	m_pluginMenuItemsRegistry->RemoveMenuItem(id);
	m_menuItemIds.erase(id);
}
