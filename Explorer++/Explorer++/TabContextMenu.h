// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include "MenuDelegate.h"
#include "../Helper/WeakPtr.h"

class ResourceLoader;
class Tab;
class TabContainer;
class TabEvents;

class TabContextMenu : public MenuBase, private MenuDelegate
{
public:
	TabContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		WeakPtr<Tab> tab, TabContainer *tabContainer, TabEvents *tabEvents,
		const ResourceLoader *resourceLoader);

private:
	void BuildMenu(const ResourceLoader *resourceLoader);

	// MenuDelegate
	bool IsItemEnabled(UINT id) const override;
	bool IsItemChecked(UINT id) const override;
	void OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown) override;

	void OnOpenParentInNewTab();
	void OnRefreshAllTabs();
	void OnRenameTab();
	void OnLockTab();
	void OnLockTabAndAddress();
	void OnCloseOtherTabs();
	void OnCloseTabsToRight();

	WeakPtr<Tab> m_tab;
	TabContainer *const m_tabContainer;
	TabEvents *const m_tabEvents;
	const ResourceLoader *const m_resourceLoader;
};
