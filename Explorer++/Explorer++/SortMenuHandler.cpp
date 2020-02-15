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

void Explorerplusplus::UpdateSortMenuItems(const Tab &tab)
{
	DeleteSortMenuItems();

	auto sortModes = tab.GetShellBrowser()->GetAvailableSortModes();
	std::vector<SortMenuItem> newSortMenuItems;

	for (SortMode sortMode : sortModes)
	{
		int SortById = DetermineSortModeMenuId(sortMode);
		int GroupById = DetermineGroupModeMenuId(sortMode);

		if (SortById != -1 && GroupById != -1)
		{
			SortMenuItem am;
			am.SortById = SortById;
			am.GroupById = GroupById;
			newSortMenuItems.push_back(am);
		}
	}

	m_sortMenuItems = newSortMenuItems;

	InsertSortMenuItems();
}

void Explorerplusplus::InsertSortMenuItems()
{
	int index = 0;

	for(const SortMenuItem &menuItem : m_sortMenuItems)
	{
		MENUITEMINFO mi;

		UINT uStringIndex = GetSortMenuItemStringIndex(menuItem.SortById);
		std::wstring menuText = ResourceHelper::LoadString(m_hLanguageModule,uStringIndex);

		mi.cbSize		= sizeof(mi);
		mi.fMask		= MIIM_ID|MIIM_STRING;
		mi.dwTypeData	= menuText.data();
		mi.wID			= menuItem.SortById;
		InsertMenuItem(m_hSortSubMenu,index,TRUE,&mi);

		ZeroMemory(&mi,sizeof(mi));
		mi.cbSize		= sizeof(mi);
		mi.fMask		= MIIM_ID|MIIM_STRING;
		mi.dwTypeData	= menuText.data();
		mi.wID			= menuItem.GroupById;
		InsertMenuItem(m_hGroupBySubMenu,index,TRUE,&mi);

		index++;
	}
}

void Explorerplusplus::DeleteSortMenuItems()
{
	for (const SortMenuItem &menuItem : m_sortMenuItems)
	{
		DeleteMenu(m_hSortSubMenu, menuItem.SortById, MF_BYCOMMAND);
		DeleteMenu(m_hGroupBySubMenu, menuItem.GroupById, MF_BYCOMMAND);
	}
}

void Explorerplusplus::SetSortMenuItemStates(const Tab &tab)
{
	const SortMode sortMode = tab.GetShellBrowser()->GetSortMode();
	BOOL bShowInGroups = tab.GetShellBrowser()->GetShowInGroups();

	/* Go through both the sort by and group by menus and
	remove all the checkmarks. Alternatively, could remember
	which items have checkmarks, and just uncheck those. */
	int nItems = GetMenuItemCount(m_hSortSubMenu);

	for (int i = 0; i < nItems; i++)
	{
		CheckMenuItem(m_hSortSubMenu, i, MF_BYPOSITION | MF_UNCHECKED);
	}

	nItems = GetMenuItemCount(m_hGroupBySubMenu);

	for (int i = 0; i < nItems; i++)
	{
		CheckMenuItem(m_hGroupBySubMenu, i, MF_BYPOSITION | MF_UNCHECKED);
	}

	HMENU activeMenu;
	HMENU inactiveMenu;
	UINT itemToCheck;
	UINT firstItem;
	UINT lastItem;

	if (bShowInGroups)
	{
		activeMenu = m_hGroupBySubMenu;
		inactiveMenu = m_hSortSubMenu;

		itemToCheck = DetermineGroupModeMenuId(sortMode);

		firstItem = IDM_GROUPBY_NAME;
		lastItem = IDM_GROUPBY_NAME + (SORT_MENU_RESOURCE_BLOCK_SIZE - 1);
	}
	else
	{
		activeMenu = m_hSortSubMenu;
		inactiveMenu = m_hGroupBySubMenu;

		itemToCheck = DetermineSortModeMenuId(sortMode);

		firstItem = IDM_SORTBY_NAME;
		lastItem = IDM_SORTBY_NAME + (SORT_MENU_RESOURCE_BLOCK_SIZE - 1);
	}

	lEnableMenuItem(inactiveMenu, IDM_SORT_ASCENDING, FALSE);
	lEnableMenuItem(inactiveMenu, IDM_SORT_DESCENDING, FALSE);

	lEnableMenuItem(activeMenu, IDM_SORT_ASCENDING, TRUE);
	lEnableMenuItem(activeMenu, IDM_SORT_DESCENDING, TRUE);

	CheckMenuRadioItem(activeMenu, firstItem, lastItem, itemToCheck, MF_BYCOMMAND);

	if (tab.GetShellBrowser()->GetSortAscending())
	{
		itemToCheck = IDM_SORT_ASCENDING;
	}
	else
	{
		itemToCheck = IDM_SORT_DESCENDING;
	}

	CheckMenuRadioItem(activeMenu, IDM_SORT_ASCENDING, IDM_SORT_DESCENDING,
		itemToCheck, MF_BYCOMMAND);
}