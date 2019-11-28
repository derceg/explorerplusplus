// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ShellBrowser/SortModes.h"
#include "SortModeHelper.h"
#include "../Helper/Controls.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include <list>

void Explorerplusplus::UpdateSortMenuItems()
{
	DeleteSortMenuItems();

	auto sortModes = m_pActiveShellBrowser->GetAvailableSortModes();
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