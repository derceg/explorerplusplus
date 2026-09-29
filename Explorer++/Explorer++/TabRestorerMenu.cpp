// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabRestorerMenu.h"
#include "AcceleratorManager.h"
#include "MainResource.h"
#include "MenuView.h"
#include "ResourceLoader.h"
#include "ShellBrowser/PreservedHistoryEntry.h"
#include "ShellIconModel.h"
#include "TabRestorer.h"
#include "../Helper/ShellHelper.h"
#include <ranges>

TabRestorerMenu::TabRestorerMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
	TabRestorer *tabRestorer, ShellIconLoader *shellIconLoader,
	const ResourceLoader *resourceLoader) :
	MenuBase(menuView, acceleratorManager),
	m_tabRestorer(tabRestorer),
	m_shellIconLoader(shellIconLoader),
	m_resourceLoader(resourceLoader)
{
	RebuildMenu();

	m_connections.push_back(tabRestorer->AddItemsChangedObserver(
		std::bind_front(&TabRestorerMenu::OnRestoreItemsChanged, this)));
}

void TabRestorerMenu::RebuildMenu()
{
	m_rootMenuView->ClearMenu();

	if (m_tabRestorer->IsEmpty())
	{
		m_rootMenuView->AppendItem(MenuItemParams(m_resourceLoader->LoadString(IDS_NO_RECENT_TABS))
				.SetEnabledCallback([] { return false; }));
		return;
	}

	for (size_t index = 0;
		const auto &closedTab : m_tabRestorer->GetClosedTabs() | std::views::take(MAX_MENU_ITEMS))
	{
		auto params = BuildMenuItemParamsForClosedTab(closedTab.get());

		if (index == 0)
		{
			params.SetAcceleratorText(GetAcceleratorTextForId(IDA_RESTORE_LAST_TAB));
		}

		m_rootMenuView->AppendItem(std::move(params));

		index++;
	}
}

MenuItemParams TabRestorerMenu::BuildMenuItemParamsForClosedTab(const PreservedTab *closedTab)
{
	const auto &preservedShellBrowser = closedTab->preservedShellBrowser;
	const auto *currentEntry =
		preservedShellBrowser.history[preservedShellBrowser.currentEntry].get();

	auto menuText = GetDisplayNameWithFallback(currentEntry->GetPidl().Raw(), SHGDN_INFOLDER);
	auto helpText = GetFolderPathForDisplayWithFallback(currentEntry->GetPidl().Raw());

	auto clickCallback = [this, closedTabId = closedTab->id]
	{
		m_tabRestorer->RestoreTabById(closedTabId);
	};

	return MenuItemParams(menuText)
		.SetIconModel(
			std::make_unique<ShellIconModel>(m_shellIconLoader, currentEntry->GetPidl().Raw()))
		.SetHelpText(helpText)
		.SetSelectedCallback(clickCallback)
		.SetMiddleClickedCallback(clickCallback);
}

void TabRestorerMenu::OnRestoreItemsChanged()
{
	RebuildMenu();
}
