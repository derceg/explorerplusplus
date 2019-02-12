// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "LuaPlugin.h"
#include "PluginMenuManager.h"
#include "UiTheming.h"
#include <boost/filesystem.hpp>

namespace Plugins
{
	class PluginManager
	{
	public:

		PluginManager(TabContainerInterface *tabContainer, PluginMenuManager *pluginMenuManager, UiTheming *uiTheming);
		~PluginManager();

		void loadAllPlugins(const boost::filesystem::path &pluginDirectory);

	private:

		static const std::wstring MANIFEST_NAME;

		bool attemptToLoadPlugin(const boost::filesystem::path &directory);
		bool registerPlugin(const boost::filesystem::path &directory, const Manifest &manifest);

		TabContainerInterface *m_tabContainer;
		PluginMenuManager *m_pluginMenuManager;
		UiTheming *m_uiTheming;

		std::vector<std::unique_ptr<Plugins::LuaPlugin>> m_plugins;
	};
}