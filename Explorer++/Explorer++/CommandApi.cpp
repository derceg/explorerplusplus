// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "CommandApi.h"

Plugins::CommandApi::CommandApi(PluginCommandManager *pluginCommandManager, int pluginId) :
	m_pluginCommandManager(pluginCommandManager),
	m_observerIdCounter(1),
	m_pluginId(pluginId)
{

}

Plugins::CommandApi::~CommandApi()
{
	for (auto item : m_commandInvokedConnections)
	{
		item.second.disconnect();
	}
}

int Plugins::CommandApi::addCommandInvokedObserver(sol::protected_function observer)
{
	if (!observer)
	{
		return -1;
	}

	auto connection = m_pluginCommandManager->AddCommandInvokedObserver([this, observer] (int pluginId, const std::wstring &name) {
		onCommandInvoked(pluginId, name, observer);
	});

	int id = m_observerIdCounter++;
	m_commandInvokedConnections.insert(std::make_pair(id, connection));

	return id;
}

void Plugins::CommandApi::onCommandInvoked(int pluginId, const std::wstring &name,
	sol::protected_function observer)
{
	if (pluginId != m_pluginId)
	{
		return;
	}

	observer(name);
}

void Plugins::CommandApi::removeCommandInvokedObserver(int id)
{
	auto itr = m_commandInvokedConnections.find(id);

	if (itr == m_commandInvokedConnections.end())
	{
		return;
	}

	itr->second.disconnect();

	m_commandInvokedConnections.erase(itr);
}