// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "MainResource.h"
#include "ShellBrowser/SortModes.h"
#include "SortModeHelper.h"
#include "../Helper/Controls.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include <list>

/*
 * Sets the arrange menu items that will be shown
 * for each folder.
 */
void Explorerplusplus::InitializeArrangeMenuItems(void)
{
	HMENU				hMainMenu;
	HMENU				hArrangeMenu;
	MENUITEMINFO		mi;
	ArrangeMenuItem_t	ArrangeMenuItem;

	hMainMenu = GetMenu(m_hContainer);
	hArrangeMenu = GetSubMenu(hMainMenu,3);

	mi.cbSize	= sizeof(mi);
	mi.fMask	= MIIM_SUBMENU;
	mi.hSubMenu	= m_hArrangeSubMenu;

	/* Insert the default arrange sub menu. This
	menu will not contain any sort menu items. */
	SetMenuItemInfo(hArrangeMenu,IDM_VIEW_SORTBY,FALSE,&mi);

	mi.cbSize	= sizeof(mi);
	mi.fMask	= MIIM_SUBMENU;
	mi.hSubMenu	= m_hGroupBySubMenu;
	SetMenuItemInfo(hArrangeMenu,IDM_VIEW_GROUPBY,FALSE,&mi);

	/* <----Real Folder----> */

	ArrangeMenuItem.SortById = IDM_SORTBY_NAME;
	ArrangeMenuItem.GroupById = IDM_GROUPBY_NAME;
	m_ArrangeMenuRealFolder.push_back(ArrangeMenuItem);

	ArrangeMenuItem.SortById = IDM_SORTBY_SIZE;
	ArrangeMenuItem.GroupById = IDM_GROUPBY_SIZE;
	m_ArrangeMenuRealFolder.push_back(ArrangeMenuItem);

	ArrangeMenuItem.SortById = IDM_SORTBY_TYPE;
	ArrangeMenuItem.GroupById = IDM_GROUPBY_TYPE;
	m_ArrangeMenuRealFolder.push_back(ArrangeMenuItem);

	ArrangeMenuItem.SortById = IDM_SORTBY_DATEMODIFIED;
	ArrangeMenuItem.GroupById = IDM_GROUPBY_DATEMODIFIED;
	m_ArrangeMenuRealFolder.push_back(ArrangeMenuItem);

	/* <----My Computer----> */

	ArrangeMenuItem.SortById = IDM_SORTBY_NAME;
	ArrangeMenuItem.GroupById = IDM_GROUPBY_NAME;
	m_ArrangeMenuMyComputer.push_back(ArrangeMenuItem);

	ArrangeMenuItem.SortById = IDM_SORTBY_TYPE;
	ArrangeMenuItem.GroupById = IDM_GROUPBY_TYPE;
	m_ArrangeMenuMyComputer.push_back(ArrangeMenuItem);

	ArrangeMenuItem.SortById = IDM_SORTBY_TOTALSIZE;
	ArrangeMenuItem.GroupById = IDM_GROUPBY_TOTALSIZE;
	m_ArrangeMenuMyComputer.push_back(ArrangeMenuItem);

	ArrangeMenuItem.SortById = IDM_SORTBY_FREESPACE;
	ArrangeMenuItem.GroupById = IDM_GROUPBY_FREESPACE;
	m_ArrangeMenuMyComputer.push_back(ArrangeMenuItem);

	ArrangeMenuItem.SortById = IDM_SORTBY_COMMENTS;
	ArrangeMenuItem.GroupById = IDM_GROUPBY_COMMENTS;
	m_ArrangeMenuMyComputer.push_back(ArrangeMenuItem);

	/* <----Control Panel----> */

	ArrangeMenuItem.SortById = IDM_SORTBY_NAME;
	ArrangeMenuItem.GroupById = IDM_GROUPBY_NAME;
	m_ArrangeMenuControlPanel.push_back(ArrangeMenuItem);

	ArrangeMenuItem.SortById = IDM_SORTBY_COMMENTS;
	ArrangeMenuItem.GroupById = IDM_GROUPBY_COMMENTS;
	m_ArrangeMenuControlPanel.push_back(ArrangeMenuItem);

	/* <----Recycle Bin----> */

	ArrangeMenuItem.SortById = IDM_SORTBY_NAME;
	ArrangeMenuItem.GroupById = IDM_GROUPBY_NAME;
	m_ArrangeMenuRecycleBin.push_back(ArrangeMenuItem);

	ArrangeMenuItem.SortById = IDM_SORTBY_ORIGINALLOCATION;
	ArrangeMenuItem.GroupById = IDM_GROUPBY_ORIGINALLOCATION;
	m_ArrangeMenuRecycleBin.push_back(ArrangeMenuItem);

	ArrangeMenuItem.SortById = IDM_SORTBY_DATEDELETED;
	ArrangeMenuItem.GroupById = IDM_GROUPBY_DATEDELETED;
	m_ArrangeMenuRecycleBin.push_back(ArrangeMenuItem);

	ArrangeMenuItem.SortById = IDM_SORTBY_SIZE;
	ArrangeMenuItem.GroupById = IDM_GROUPBY_SIZE;
	m_ArrangeMenuRecycleBin.push_back(ArrangeMenuItem);

	ArrangeMenuItem.SortById = IDM_SORTBY_TYPE;
	ArrangeMenuItem.GroupById = IDM_GROUPBY_TYPE;
	m_ArrangeMenuRecycleBin.push_back(ArrangeMenuItem);

	ArrangeMenuItem.SortById = IDM_SORTBY_DATEMODIFIED;
	ArrangeMenuItem.GroupById = IDM_GROUPBY_DATEMODIFIED;
	m_ArrangeMenuRecycleBin.push_back(ArrangeMenuItem);
}

/*
 * Sets the current arrange menu used based on
 * the current folder.
 */
void Explorerplusplus::SetActiveArrangeMenuItems(void)
{
	if(CompareVirtualFolders(m_CurrentDirectory, CSIDL_DRIVES))
	{
		m_pActiveArrangeMenuItems = &m_ArrangeMenuMyComputer;
	}
	else if(CompareVirtualFolders(m_CurrentDirectory, CSIDL_CONTROLS))
	{
		m_pActiveArrangeMenuItems = &m_ArrangeMenuControlPanel;
	}
	else if(CompareVirtualFolders(m_CurrentDirectory, CSIDL_BITBUCKET))
	{
		m_pActiveArrangeMenuItems = &m_ArrangeMenuRecycleBin;
	}
	else
	{
		m_pActiveArrangeMenuItems = &m_ArrangeMenuRealFolder;
	}
}

/*
 * Inserts the current arrange menu items onto the
 * specified menu.
 */
int Explorerplusplus::InsertArrangeMenuItems(HMENU hMenu)
{
	MENUITEMINFO						mi;
	std::list<ArrangeMenuItem_t>::iterator	itr;
	TCHAR								szStringTemp[32];
	UINT								uStringIndex;
	int									i = 0;

	for(itr = m_pActiveArrangeMenuItems->begin();itr != m_pActiveArrangeMenuItems->end();itr++)
	{
		uStringIndex = GetArrangeMenuItemStringIndex(itr->SortById);
		LoadString(m_hLanguageModule,uStringIndex,szStringTemp,SIZEOF_ARRAY(szStringTemp));

		mi.cbSize		= sizeof(mi);
		mi.fMask		= MIIM_ID|MIIM_STRING;
		mi.dwTypeData	= szStringTemp;
		mi.wID			= itr->SortById;
		InsertMenuItem(hMenu,i,TRUE,&mi);
		InsertMenuItem(m_hArrangeSubMenuRClick,i,TRUE,&mi);

		ZeroMemory(&mi,sizeof(mi));
		mi.cbSize		= sizeof(mi);
		mi.fMask		= MIIM_ID|MIIM_STRING;
		mi.dwTypeData	= szStringTemp;
		mi.wID			= itr->GroupById;
		InsertMenuItem(m_hGroupBySubMenu,i,TRUE,&mi);
		InsertMenuItem(m_hGroupBySubMenuRClick,i,TRUE,&mi);

		i++;
	}

	return i;
}

/*
 * Removes the previous arrange menu items from the menu.
 */
void Explorerplusplus::DeletePreviousArrangeMenuItems(void)
{
	for(int i = m_iMaxArrangeMenuItem - 1;i >= 0;i--)
	{
		DeleteMenu(m_hArrangeSubMenu,i,MF_BYPOSITION);
		DeleteMenu(m_hArrangeSubMenuRClick,i,MF_BYPOSITION);
		DeleteMenu(m_hGroupBySubMenu,i,MF_BYPOSITION);
		DeleteMenu(m_hGroupBySubMenuRClick,i,MF_BYPOSITION);
	}
}

/*
 * Updates the arrange menu with the new items.
 */
void Explorerplusplus::UpdateArrangeMenuItems(void)
{
	std::list<int>			SortModes;
	std::list<int>::iterator	itr;
	ArrangeMenuItem_t		am;
	int						SortById;
	int						GroupById;

	DeletePreviousArrangeMenuItems();

	SortModes = m_pActiveShellBrowser->GetAvailableSortModes();

	m_ArrangeList.clear();

	if(SortModes.size() != 0)
	{
		for(itr = SortModes.begin();itr != SortModes.end();itr++)
		{
			SortMode sortMode = SortMode::_from_integral(*itr);

			SortById = DetermineSortModeMenuId(sortMode);
			GroupById = DetermineGroupModeMenuId(sortMode);

			if(SortById != -1 && GroupById != -1)
			{
				am.SortById		= SortById;
				am.GroupById	= GroupById;

				m_ArrangeList.push_back(am);
			}
		}

		m_pActiveArrangeMenuItems = &m_ArrangeList;
	}
	else
	{
		SetActiveArrangeMenuItems();
	}

	SortModes.clear();

	m_iMaxArrangeMenuItem = InsertArrangeMenuItems(m_hArrangeSubMenu);
}