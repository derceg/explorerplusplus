// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Plugins/CommandApi/Events/CommandInvoked.h"
#include <sol/sol.hpp>

Plugins::CommandInvoked::CommandInvoked(PluginCommandManager *pluginCommandManager, int pluginId) :
	m_pluginCommandManager(pluginCommandManager),
	m_pluginId(pluginId)
{
}

boost::signals2::connection Plugins::CommandInvoked::connectObserver(
	sol::protected_function observer, sol::this_state state)
{
	UNREFERENCED_PARAMETER(state);

	return m_pluginCommandManager->AddCommandInvokedObserver(
		[this, observer](int pluginId, const std::wstring &name)
		{ onCommandInvoked(pluginId, name, observer); });
}

void Plugins::CommandInvoked::onCommandInvoked(int pluginId, const std::wstring &name,
	sol::protected_function observer)
{
	if (pluginId != m_pluginId)
	{
		return;
	}

	observer(name);
}
