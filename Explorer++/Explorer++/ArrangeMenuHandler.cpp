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

void Explorerplusplus::InitializeArrangeMenuItems()
{
	HMENU hMainMenu = GetMenu(m_hContainer);

	// Insert the default arrange sub menu. This menu will not contain any sort
	// menu items.
	MENUITEMINFO mi;
	mi.cbSize	= sizeof(mi);
	mi.fMask	= MIIM_SUBMENU;
	mi.hSubMenu	= m_hArrangeSubMenu;
	SetMenuItemInfo(hMainMenu,IDM_VIEW_SORTBY,FALSE,&mi);

	mi.cbSize	= sizeof(mi);
	mi.fMask	= MIIM_SUBMENU;
	mi.hSubMenu	= m_hGroupBySubMenu;
	SetMenuItemInfo(hMainMenu,IDM_VIEW_GROUPBY,FALSE,&mi);
}

void Explorerplusplus::InsertArrangeMenuItems(HMENU hMenu)
{
	int index = 0;

	for(const ArrangeMenuItem_t &menuItem : m_ArrangeMenuItems)
	{
		MENUITEMINFO mi;

		UINT uStringIndex = GetArrangeMenuItemStringIndex(menuItem.SortById);
		std::wstring menuText = ResourceHelper::LoadString(m_hLanguageModule,uStringIndex);

		mi.cbSize		= sizeof(mi);
		mi.fMask		= MIIM_ID|MIIM_STRING;
		mi.dwTypeData	= menuText.data();
		mi.wID			= menuItem.SortById;
		InsertMenuItem(hMenu,index,TRUE,&mi);
		InsertMenuItem(m_hArrangeSubMenuRClick,index,TRUE,&mi);

		ZeroMemory(&mi,sizeof(mi));
		mi.cbSize		= sizeof(mi);
		mi.fMask		= MIIM_ID|MIIM_STRING;
		mi.dwTypeData	= menuText.data();
		mi.wID			= menuItem.GroupById;
		InsertMenuItem(m_hGroupBySubMenu,index,TRUE,&mi);
		InsertMenuItem(m_hGroupBySubMenuRClick,index,TRUE,&mi);

		index++;
	}
}

void Explorerplusplus::DeleteArrangeMenuItems()
{
	for (const ArrangeMenuItem_t &menuItem : m_ArrangeMenuItems)
	{
		DeleteMenu(m_hArrangeSubMenu, menuItem.SortById, MF_BYCOMMAND);
		DeleteMenu(m_hArrangeSubMenuRClick, menuItem.SortById, MF_BYCOMMAND);
		DeleteMenu(m_hGroupBySubMenu, menuItem.GroupById, MF_BYCOMMAND);
		DeleteMenu(m_hGroupBySubMenuRClick, menuItem.GroupById, MF_BYCOMMAND);
	}
}

void Explorerplusplus::UpdateArrangeMenuItems()
{
	DeleteArrangeMenuItems();

	auto sortModes = m_pActiveShellBrowser->GetAvailableSortModes();

	m_ArrangeMenuItems.clear();

	for(SortMode sortMode : sortModes)
	{
		int SortById = DetermineSortModeMenuId(sortMode);
		int GroupById = DetermineGroupModeMenuId(sortMode);

		if(SortById != -1 && GroupById != -1)
		{
			ArrangeMenuItem_t am;
			am.SortById = SortById;
			am.GroupById = GroupById;
			m_ArrangeMenuItems.push_back(am);
		}
	}

	InsertArrangeMenuItems(m_hArrangeSubMenu);
}