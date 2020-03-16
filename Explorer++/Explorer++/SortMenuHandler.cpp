// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/SortModes.h"
#include "SortModeHelper.h"
#include "../Helper/MenuHelper.h"

const int SORT_MENU_RESOURCE_BLOCK_SIZE = 1000;

Explorerplusplus::SortMenus Explorerplusplus::BuildSortByAndGroupByMenus(const Tab &tab)
{
	auto sortByMenu = CreateDefaultSortByGroupByMenu();
	auto groupByMenu = CreateDefaultSortByGroupByMenu();

	auto sortModes = tab.GetShellBrowser()->GetAvailableSortModes();
	int position = 0;

	for (SortMode sortMode : sortModes)
	{
		int sortById = DetermineSortModeMenuId(sortMode);
		int groupById = DetermineGroupModeMenuId(sortMode);

		UINT stringIndex = GetSortMenuItemStringIndex(sortById);
		std::wstring menuText = ResourceHelper::LoadString(m_hLanguageModule, stringIndex);

		MenuHelper::AddStringItem(sortByMenu.get(), sortById, menuText, position, TRUE);
		MenuHelper::AddStringItem(groupByMenu.get(), groupById, menuText, position, TRUE);

		position++;
	}

	SetSortMenuItemStates(sortByMenu.get(), groupByMenu.get(), tab);

	return { std::move(sortByMenu), std::move(groupByMenu) };
}

wil::unique_hmenu Explorerplusplus::CreateDefaultSortByGroupByMenu()
{
	wil::unique_hmenu menu(CreatePopupMenu());

	MenuHelper::AddSeparator(menu.get());

	std::wstring sortAscending =
		ResourceHelper::LoadString(m_hLanguageModule, IDS_MENU_SORT_ASCENDING);
	MenuHelper::AddStringItem(menu.get(), IDM_SORT_ASCENDING, sortAscending);

	std::wstring sortDescending =
		ResourceHelper::LoadString(m_hLanguageModule, IDS_MENU_SORT_DESCENDING);
	MenuHelper::AddStringItem(menu.get(), IDM_SORT_DESCENDING, sortDescending);

	MenuHelper::AddSeparator(menu.get());

	std::wstring sortByMore = ResourceHelper::LoadString(m_hLanguageModule, IDS_MENU_SORT_MORE);
	MenuHelper::AddStringItem(menu.get(), IDM_SORTBY_MORE, sortByMore);

	return menu;
}

void Explorerplusplus::SetSortMenuItemStates(HMENU sortByMenu, HMENU groupByMenu, const Tab &tab)
{
	const SortMode sortMode = tab.GetShellBrowser()->GetSortMode();
	BOOL showInGroups = tab.GetShellBrowser()->GetShowInGroups();

	HMENU activeMenu;
	HMENU inactiveMenu;
	UINT itemToCheck;
	UINT firstItem;
	UINT lastItem;

	if (showInGroups)
	{
		activeMenu = groupByMenu;
		inactiveMenu = sortByMenu;

		itemToCheck = DetermineGroupModeMenuId(sortMode);

		firstItem = IDM_GROUPBY_NAME;
		lastItem = IDM_GROUPBY_NAME + (SORT_MENU_RESOURCE_BLOCK_SIZE - 1);
	}
	else
	{
		activeMenu = sortByMenu;
		inactiveMenu = groupByMenu;

		itemToCheck = DetermineSortModeMenuId(sortMode);

		firstItem = IDM_SORTBY_NAME;
		lastItem = IDM_SORTBY_NAME + (SORT_MENU_RESOURCE_BLOCK_SIZE - 1);
	}

	MenuHelper::EnableItem(inactiveMenu, IDM_SORT_ASCENDING, FALSE);
	MenuHelper::EnableItem(inactiveMenu, IDM_SORT_DESCENDING, FALSE);

	MenuHelper::EnableItem(activeMenu, IDM_SORT_ASCENDING, TRUE);
	MenuHelper::EnableItem(activeMenu, IDM_SORT_DESCENDING, TRUE);

	CheckMenuRadioItem(activeMenu, firstItem, lastItem, itemToCheck, MF_BYCOMMAND);

	if (tab.GetShellBrowser()->GetSortAscending())
	{
		itemToCheck = IDM_SORT_ASCENDING;
	}
	else
	{
		itemToCheck = IDM_SORT_DESCENDING;
	}

	CheckMenuRadioItem(
		activeMenu, IDM_SORT_ASCENDING, IDM_SORT_DESCENDING, itemToCheck, MF_BYCOMMAND);
}