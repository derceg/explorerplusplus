// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PluginCommandManager.h"
#include <boost/algorithm/string.hpp>

Plugins::PluginCommandManager::PluginCommandManager(HACCEL *acceleratorTable, int startId, int endId) :
	m_acceleratorTable(acceleratorTable),
	m_startId(startId),
	m_endId(endId),
	m_idCounter(startId)
{

}

Plugins::PluginCommandManager::~PluginCommandManager()
{

}

void Plugins::PluginCommandManager::addCommands(int pluginId, const std::vector<Command> &commands)
{
	int numAccelerators = CopyAcceleratorTable(*m_acceleratorTable, nullptr, 0);

	std::vector<ACCEL> accelerators(numAccelerators);
	CopyAcceleratorTable(*m_acceleratorTable, &accelerators[0], static_cast<int>(accelerators.size()));

	std::unordered_map<int, PluginCommand> registeredCommands;

	for (auto command : commands)
	{
		if (!command.accelerator)
		{
			// If the command wasn't parsed successfully, ignore it.
			continue;
		}

		auto itr = std::find_if(accelerators.begin(), accelerators.end(), [command] (const ACCEL &accel) {
			return (accel.fVirt & ~FNOINVERT) == command.accelerator->modifiers && accel.key == command.accelerator->key;
		});

		if (itr != accelerators.end())
		{
			// Accelerators are overridden using a different mechanism, so it's
			// not possible to change them here.
			continue;
		}

		auto id = generateId();

		if (!id)
		{
			// There are only a fixed number of accelerator items
			// available. As accelerators can't be removed, if there are
			// no more IDs left, no new accelerators can be created. The
			// limit shouldn't be hit in practice.
			break;
		}

		ACCEL newAccel;
		newAccel.fVirt = command.accelerator->modifiers;
		newAccel.key = command.accelerator->key;
		newAccel.cmd = static_cast<WORD>(*id);
		accelerators.push_back(newAccel);

		PluginCommand pluginCommand;
		pluginCommand.pluginId = pluginId;
		pluginCommand.name = command.name;
		registeredCommands.insert(std::make_pair(*id, pluginCommand));
	}

	HACCEL newAcceleratorTable = CreateAcceleratorTable(&accelerators[0], static_cast<int>(accelerators.size()));

	if (newAcceleratorTable == nullptr)
	{
		return;
	}

	m_registeredCommands.insert(registeredCommands.begin(), registeredCommands.end());

	DestroyAcceleratorTable(*m_acceleratorTable);
	*m_acceleratorTable = newAcceleratorTable;
}

boost::optional<int> Plugins::PluginCommandManager::generateId()
{
	if (m_idCounter >= m_endId)
	{
		return boost::none;
	}

	return m_idCounter++;
}

boost::signals2::connection Plugins::PluginCommandManager::AddCommandInvokedObserver(const CommandInvokedSignal::slot_type &observer)
{
	return m_commandInvokedSignal.connect(observer);
}

void Plugins::PluginCommandManager::onAcceleratorPressed(int acceleratorId)
{
	auto itr = m_registeredCommands.find(acceleratorId);

	if (itr == m_registeredCommands.end())
	{
		return;
	}

	m_commandInvokedSignal(itr->second.pluginId, itr->second.name);
}