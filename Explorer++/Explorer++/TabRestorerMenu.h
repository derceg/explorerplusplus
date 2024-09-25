// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include <boost/core/noncopyable.hpp>
#include <unordered_map>

struct PreservedTab;
class ShellIconLoader;
class TabRestorer;

class TabRestorerMenu : public MenuBase, private boost::noncopyable
{
public:
	TabRestorerMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		TabRestorer *tabRestorer, ShellIconLoader *shellIconLoader, UINT menuStartId,
		UINT menuEndId);

private:
	static const int MAX_MENU_ITEMS = 10;

	void RebuildMenu();
	void AddMenuItemForClosedTab(const PreservedTab *closedTab, bool addAcceleratorText);

	void OnRestoreItemsChanged();

	void OnMenuItemSelected(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown);
	void OnMenuItemMiddleClicked(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown);
	void RestoreTabForMenuItem(UINT menuItemId);

	TabRestorer *const m_tabRestorer;
	ShellIconLoader *const m_shellIconLoader;
	const UINT m_menuStartId;
	const UINT m_menuEndId;
	UINT m_idCounter;

	std::vector<boost::signals2::scoped_connection> m_connections;

	// Maps between menu item IDs and closed tab IDs.
	std::unordered_map<UINT, int> m_menuItemMappings;
};
