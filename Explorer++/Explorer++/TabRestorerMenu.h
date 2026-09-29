// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include <boost/core/noncopyable.hpp>

class MenuItemParams;
struct PreservedTab;
class ResourceLoader;
class ShellIconLoader;
class TabRestorer;

class TabRestorerMenu : public MenuBase, private boost::noncopyable
{
public:
	TabRestorerMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		TabRestorer *tabRestorer, ShellIconLoader *shellIconLoader,
		const ResourceLoader *resourceLoader);

private:
	static const int MAX_MENU_ITEMS = 10;

	void RebuildMenu();
	MenuItemParams BuildMenuItemParamsForClosedTab(const PreservedTab *closedTab);

	void OnRestoreItemsChanged();

	TabRestorer *const m_tabRestorer;
	ShellIconLoader *const m_shellIconLoader;
	const ResourceLoader *const m_resourceLoader;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
