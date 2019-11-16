// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Accelerator.h"
#include "Manifest.h"
#include <boost/optional.hpp>
#include <boost/signals2.hpp>
#include <unordered_map>

namespace Plugins
{
	class PluginCommandManager
	{
	public:

		typedef boost::signals2::signal<void(int, const std::wstring &)> CommandInvokedSignal;

		PluginCommandManager(HACCEL *acceleratorTable, int startId, int endId);

		void addCommands(int pluginId, const std::vector<Command> &commands);

		boost::signals2::connection AddCommandInvokedObserver(const CommandInvokedSignal::slot_type &observer);

		void onAcceleratorPressed(int acceleratorId);

	private:

		struct PluginCommand
		{
			int pluginId;
			std::wstring name;
		};

		HACCEL *m_acceleratorTable;
		const int m_startId;
		const int m_endId;

		int m_idCounter;

		std::unordered_map<int, PluginCommand> m_registeredCommands;

		boost::optional<int> generateId();

		CommandInvokedSignal m_commandInvokedSignal;
	};
}