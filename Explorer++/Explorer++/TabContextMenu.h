// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include <boost/signals2.hpp>
#include <vector>

class ResourceLoader;
class Tab;
class TabContainerImpl;
class TabEvents;

class TabContextMenu : public MenuBase
{
public:
	TabContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager, Tab *tab,
		TabContainerImpl *tabContainer, TabEvents *tabEvents, const ResourceLoader *resourceLoader);

private:
	void BuildMenu(const ResourceLoader *resourceLoader);

	void OnMenuItemSelected(UINT menuItemId);
	void OnOpenParentInNewTab();
	void OnRefreshAllTabs();
	void OnRenameTab();
	void OnLockTab();
	void OnLockTabAndAddress();
	void OnCloseOtherTabs();
	void OnCloseTabsToRight();

	void OnTabClosed(const Tab &tab);

	Tab *m_tab = nullptr;
	TabContainerImpl *const m_tabContainer;
	TabEvents *const m_tabEvents;
	const ResourceLoader *const m_resourceLoader;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
