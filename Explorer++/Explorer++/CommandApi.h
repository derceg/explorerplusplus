// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "PluginCommandManager.h"
#include "../ThirdParty/Sol/sol.hpp"

namespace Plugins
{
	class CommandApi
	{
	public:

		CommandApi(PluginCommandManager *pluginCommandManager, int pluginId);
		~CommandApi();

		int addCommandInvokedObserver(sol::protected_function observer);
		void removeCommandInvokedObserver(int id);

	private:

		void onCommandInvoked(int pluginId, const std::wstring &name, sol::protected_function observer);

		PluginCommandManager *m_pluginCommandManager;
		int m_observerIdCounter;
		std::unordered_map<int, boost::signals2::connection> m_commandInvokedConnections;
		int m_pluginId;
	};
}