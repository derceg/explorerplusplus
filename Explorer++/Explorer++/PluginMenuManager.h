// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/optional.hpp>
#include <boost/signals2.hpp>
#include <set>

namespace Plugins
{
	class PluginMenuManager
	{
	public:

		typedef boost::signals2::signal<void(int)> PluginMenuClickedSignal;

		PluginMenuManager(HWND mainWindow, int startId, int endId);

		boost::optional<int> AddItemToMainMenu(const std::wstring &text);
		void RemoveItemFromMainMenu(int menuItemId);

		boost::signals2::connection AddMenuClickedObserver(const PluginMenuClickedSignal::slot_type &observer);

		void OnMenuItemClicked(int menuItemId);

	private:

		boost::optional<int> GeneratePluginMenuItemId();
		void ReleasePluginMenuItemId(int id);

		const HWND m_mainWindow;
		const int m_startId;
		const int m_endId;

		std::set<int> m_freeMenuItemIds;

		PluginMenuClickedSignal	m_menuClickedSignal;
	};
}