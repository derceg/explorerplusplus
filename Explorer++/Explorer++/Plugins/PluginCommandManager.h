// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>
#include <optional>
#include <unordered_map>

class AcceleratorManager;

namespace Plugins
{

struct Command;

class PluginCommandManager
{
public:
	typedef boost::signals2::signal<void(int, const std::wstring &)> CommandInvokedSignal;

	PluginCommandManager(AcceleratorManager *acceleratorManager, int startId, int endId);

	void addCommands(int pluginId, const std::vector<Command> &commands);

	boost::signals2::connection AddCommandInvokedObserver(
		const CommandInvokedSignal::slot_type &observer);

	void onAcceleratorPressed(int acceleratorId);

private:
	struct PluginCommand
	{
		int pluginId;
		std::wstring name;
	};

	AcceleratorManager *const m_acceleratorManager;
	const int m_startId;
	const int m_endId;

	int m_idCounter;

	std::unordered_map<int, PluginCommand> m_registeredCommands;

	CommandInvokedSignal m_commandInvokedSignal;
};

}
