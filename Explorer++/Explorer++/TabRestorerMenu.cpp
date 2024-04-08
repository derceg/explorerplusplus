// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabRestorerMenu.h"
#include "MainResource.h"
#include "MenuView.h"
#include "ResourceHelper.h"
#include "ShellBrowser/PreservedHistoryEntry.h"
#include "TabRestorer.h"
#include "../Helper/ImageHelper.h"
#include "../Helper/ShellHelper.h"
#include <ranges>

TabRestorerMenu::TabRestorerMenu(MenuView *menuView, TabRestorer *tabRestorer,
	HINSTANCE resourceInstance, UINT menuStartId, UINT menuEndId) :
	MenuBase(menuView),
	m_resourceInstance(resourceInstance),
	m_tabRestorer(tabRestorer),
	m_menuStartId(menuStartId),
	m_menuEndId(menuEndId),
	m_idCounter(menuStartId)
{
	FAIL_FAST_IF_FAILED(SHGetImageList(SHIL_SYSSMALL, IID_PPV_ARGS(&m_systemImageList)));
	FAIL_FAST_IF_FAILED(GetDefaultFolderIconIndex(m_defaultFolderIconIndex));

	m_connections.push_back(tabRestorer->AddItemsChangedObserver(
		std::bind_front(&TabRestorerMenu::OnRestoreItemsChanged, this)));
	m_connections.push_back(m_menuView->AddItemSelectedObserver(
		std::bind_front(&TabRestorerMenu::OnMenuItemSelected, this)));
	m_connections.push_back(m_menuView->AddItemMiddleClickedObserver(
		std::bind_front(&TabRestorerMenu::OnMenuItemMiddleClicked, this)));

	RebuildMenu();
}

void TabRestorerMenu::RebuildMenu()
{
	m_menuView->ClearMenu();
	m_idCounter = m_menuStartId;
	m_menuItemMappings.clear();

	if (m_tabRestorer->GetClosedTabs().empty())
	{
		auto id = m_idCounter++;
		m_menuView->AppendItem(id,
			ResourceHelper::LoadString(m_resourceInstance, IDS_NO_RECENT_TABS));
		m_menuView->EnableItem(id, false);
		return;
	}

	for (size_t index = 0; const auto &closedTab :
		 m_tabRestorer->GetClosedTabs() | std::ranges::views::take(MAX_MENU_ITEMS))
	{
		AddMenuItemForClosedTab(closedTab.get(), index == 0);
		index++;
	}
}

void TabRestorerMenu::AddMenuItemForClosedTab(const PreservedTab *closedTab,
	bool addAcceleratorText)
{
	UINT id = m_idCounter++;

	if (id >= m_menuEndId)
	{
		return;
	}

	auto currentEntry = closedTab->history.at(closedTab->currentEntry).get();

	std::wstring menuText = currentEntry->displayName;

	if (addAcceleratorText)
	{
		// TODO: As accelerator key bindings can be customized, this should be dynamically looked
		// up.
		menuText += L"\tCtrl+Shift+T";
	}

	wil::unique_hbitmap bitmap;
	auto iconIndex = currentEntry->systemIconIndex;

	if (iconIndex)
	{
		bitmap = ImageHelper::ImageListIconToBitmap(m_systemImageList.get(), *iconIndex);
	}
	else
	{
		bitmap =
			ImageHelper::ImageListIconToBitmap(m_systemImageList.get(), m_defaultFolderIconIndex);
	}

	m_menuView->AppendItem(id, menuText, std::move(bitmap), currentEntry->fullPathForDisplay);

	auto [itr, didInsert] = m_menuItemMappings.insert({ id, closedTab->id });
	DCHECK(didInsert);
}

void TabRestorerMenu::OnRestoreItemsChanged()
{
	RebuildMenu();
}

void TabRestorerMenu::OnMenuItemSelected(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	UNREFERENCED_PARAMETER(isCtrlKeyDown);
	UNREFERENCED_PARAMETER(isShiftKeyDown);

	RestoreTabForMenuItem(menuItemId);
}

void TabRestorerMenu::OnMenuItemMiddleClicked(UINT menuItemId, bool isCtrlKeyDown,
	bool isShiftKeyDown)
{
	UNREFERENCED_PARAMETER(isCtrlKeyDown);
	UNREFERENCED_PARAMETER(isShiftKeyDown);

	RestoreTabForMenuItem(menuItemId);
}

void TabRestorerMenu::RestoreTabForMenuItem(UINT menuItemId)
{
	auto itr = m_menuItemMappings.find(menuItemId);
	CHECK(itr != m_menuItemMappings.end());

	m_tabRestorer->RestoreTabById(itr->second);
}
