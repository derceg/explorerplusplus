// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include "../Helper/WeakPtr.h"

class ResourceLoader;
class Tab;
class TabBar;
class TabEvents;

class TabContextMenu : public MenuBase
{
public:
	enum class Item
	{
		NewTab,
		DuplicateTab,
		OpenParentInNewTab,
		Refresh,
		RefreshAll,
		RenameTab,
		LockTab,
		LockTabAndAddress,
		CloseTab,
		CloseOtherTabs,
		CloseTabsToRight
	};

	TabContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		WeakPtr<Tab> tab, TabBar *tabBar, TabEvents *tabEvents,
		const ResourceLoader *resourceLoader);

private:
	void BuildMenu(const ResourceLoader *resourceLoader);

	void OnDuplicateTab();
	void OnOpenParentInNewTab();
	void OnRefresh();
	void OnRefreshAllTabs();
	void OnRenameTab();
	void OnLockTab();
	void OnLockTabAndAddress();
	void OnCloseTab();
	void OnCloseOtherTabs();
	void OnCloseTabsToRight();

	WeakPtr<Tab> m_tab;
	TabBar *const m_tabBar;
	TabEvents *const m_tabEvents;
	const ResourceLoader *const m_resourceLoader;
};
