// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Plugins/PluginMenuManager.h"
#include "../ThirdParty/Sol/forward.hpp"
#include <boost/signals2.hpp>
#include <optional>
#include <unordered_map>

namespace Plugins
{
	class MenuApi
	{
	public:

		MenuApi(PluginMenuManager *pluginMenuManager);
		~MenuApi();

		std::optional<int> create(const std::wstring &text, sol::protected_function callback);
		void remove(int menuItemId);

	private:

		void onMenuItemClicked(int menuItemId);

		PluginMenuManager *m_pluginMenuManager;

		std::vector<boost::signals2::scoped_connection> m_connections;

		std::unordered_map<int, sol::protected_function> m_pluginMenuItems;
	};
}