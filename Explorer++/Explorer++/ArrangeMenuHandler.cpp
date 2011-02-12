/******************************************************************
 *
 * Project: Explorer++
 * File: ArrangeMenuHandler.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles arrange menu items.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "Explorer++.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/Controls.h"
#include "../Helper/Bookmark.h"
#include "MainResource.h"


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

	ArrangeMenuItem.GroupById = IDM_SORTBY_SIZE;
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
 * Returns the index in the string table of the text
 * for the specified arrange menu item.
 */
UINT Explorerplusplus::GetArrangeMenuItemStringIndex(UINT uItemId)
{
	switch(uItemId)
	{
	case IDM_SORTBY_NAME:
		return IDS_SORTBY_NAME;
		break;

	case IDM_SORTBY_SIZE:
		return IDS_SORTBY_SIZE;
		break;

	case IDM_SORTBY_TYPE:
		return IDS_SORTBY_TYPE;
		break;

	case IDM_SORTBY_DATEMODIFIED:
		return IDS_SORTBY_DATEMODIFIED;
		break;

	case IDM_SORTBY_TOTALSIZE:
		return IDS_SORTBY_TOTALSIZE;
		break;

	case IDM_SORTBY_FREESPACE:
		return IDS_SORTBY_FREESPACE;
		break;

	case IDM_SORTBY_DATEDELETED:
		return IDS_SORTBY_DATEDELETED;
		break;

	case IDM_SORTBY_ORIGINALLOCATION:
		return IDS_SORTBY_ORIGINALLOCATION;
		break;

	case IDM_SORTBY_ATTRIBUTES:
		return IDS_SORTBY_ATTRIBUTES;
		break;

	case IDM_SORTBY_REALSIZE:
		return IDS_SORTBY_REALSIZE;
		break;

	case IDM_SORTBY_SHORTNAME:
		return IDS_SORTBY_SHORTNAME;
		break;

	case IDM_SORTBY_OWNER:
		return IDS_SORTBY_OWNER;
		break;

	case IDM_SORTBY_PRODUCTNAME:
		return IDS_SORTBY_PRODUCTNAME;
		break;

	case IDM_SORTBY_COMPANY:
		return IDS_SORTBY_COMPANY;
		break;

	case IDM_SORTBY_DESCRIPTION:
		return IDS_SORTBY_DESCRIPTION;
		break;

	case IDM_SORTBY_FILEVERSION:
		return IDS_SORTBY_FILEVERSION;
		break;

	case IDM_SORTBY_PRODUCTVERSION:
		return IDS_SORTBY_PRODUCTVERSION;
		break;

	case IDM_SORTBY_SHORTCUTTO:
		return IDS_SORTBY_SHORTCUTTO;
		break;

	case IDM_SORTBY_HARDLINKS:
		return IDS_SORTBY_HARDLINKS;
		break;

	case IDM_SORTBY_EXTENSION:
		return IDS_SORTBY_EXTENSION;
		break;

	case IDM_SORTBY_CREATED:
		return IDS_SORTBY_CREATED;
		break;

	case IDM_SORTBY_ACCESSED:
		return IDS_SORTBY_ACCESSED;
		break;

	case IDM_SORTBY_TITLE:
		return IDS_SORTBY_TITLE;
		break;

	case IDM_SORTBY_SUBJECT:
		return IDS_SORTBY_SUBJECT;
		break;

	case IDM_SORTBY_AUTHOR:
		return IDS_SORTBY_AUTHOR;
		break;

	case IDM_SORTBY_KEYWORDS:
		return IDS_SORTBY_KEYWORDS;
		break;

	case IDM_SORTBY_COMMENTS:
		return IDS_SORTBY_COMMENT;
		break;

	case IDM_SORTBY_CAMERAMODEL:
		return IDS_SORTBY_CAMERAMODEL;
		break;

	case IDM_SORTBY_DATETAKEN:
		return IDS_SORTBY_DATETAKEN;
		break;

	case IDM_SORTBY_WIDTH:
		return IDS_SORTBY_WIDTH;
		break;

	case IDM_SORTBY_HEIGHT:
		return IDS_SORTBY_HEIGHT;
		break;

	case IDM_SORTBY_VIRTUALCOMMENTS:
		return IDS_SORTBY_COMMENT;
		break;

	case IDM_SORTBY_FILESYSTEM:
		return IDS_SORTBY_FILESYSTEM;
		break;

	case IDM_SORTBY_VIRTUALTYPE:
		return IDS_SORTBY_TYPE;
		break;

	case IDM_SORTBY_NUMPRINTERDOCUMENTS:
		return IDS_SORTBY_NUMPRINTERDOCUMENTS;
		break;

	case IDM_SORTBY_PRINTERSTATUS:
		return IDS_SORTBY_PRINTERSTATUS;
		break;

	case IDM_SORTBY_PRINTERCOMMENTS:
		return IDS_SORTBY_COMMENT;
		break;

	case IDM_SORTBY_PRINTERLOCATION:
		return IDS_SORTBY_PRINTERLOCATION;
		break;

	case IDM_SORTBY_NETWORKADAPTER_STATUS:
		return IDS_SORTBY_NETWORKADAPTERSTATUS;
		break;
	}

	return 0;
}

/*
 * Sets the current arrange menu used based on
 * the current folder.
 */
void Explorerplusplus::SetActiveArrangeMenuItems(void)
{
	if(CompareVirtualFolders(CSIDL_DRIVES))
	{
		m_pActiveArrangeMenuItems = &m_ArrangeMenuMyComputer;
	}
	else if(CompareVirtualFolders(CSIDL_CONTROLS))
	{
		m_pActiveArrangeMenuItems = &m_ArrangeMenuControlPanel;
	}
	else if(CompareVirtualFolders(CSIDL_BITBUCKET))
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
	list<ArrangeMenuItem_t>::iterator	itr;
	TCHAR								szStringTemp[32];
	UINT								uStringIndex;
	int									i = 0;

	for(itr = m_pActiveArrangeMenuItems->begin();itr != m_pActiveArrangeMenuItems->end();itr++)
	{
		uStringIndex = GetArrangeMenuItemStringIndex(itr->SortById);
		LoadString(g_hLanguageModule,uStringIndex,szStringTemp,SIZEOF_ARRAY(szStringTemp));

		mi.cbSize		= sizeof(mi);
		mi.fMask		= MIIM_ID|MIIM_STRING;
		mi.dwTypeData	= szStringTemp;
		mi.wID			= itr->SortById;
		InsertMenuItem(hMenu,i,TRUE,&mi);

		ZeroMemory(&mi,sizeof(mi));
		mi.cbSize		= sizeof(mi);
		mi.fMask		= MIIM_ID|MIIM_STRING;
		mi.dwTypeData	= szStringTemp;
		mi.wID			= itr->GroupById;
		InsertMenuItem(m_hGroupBySubMenu,i,TRUE,&mi);

		i++;
	}

	return i;
}

/*
 * Translates the specified sort mode into its
 * associated menu id.
 */
int Explorerplusplus::DetermineSortModeMenuId(int SortMode)
{
	switch(SortMode)
	{
		case FSM_NAME:
			return IDM_SORTBY_NAME;
			break;

		case FSM_DATEMODIFIED:
			return IDM_SORTBY_DATEMODIFIED;
			break;

		case FSM_SIZE:
			return IDM_SORTBY_SIZE;
			break;

		case FSM_TYPE:
			return IDM_SORTBY_TYPE;
			break;

		case FSM_TOTALSIZE:
			return IDM_SORTBY_TOTALSIZE;
			break;

		case FSM_FREESPACE:
			return IDM_SORTBY_FREESPACE;
			break;

		case FSM_COMMENTS:
			return IDM_SORTBY_COMMENTS;
			break;

		case FSM_DATEDELETED:
			return IDM_SORTBY_DATEDELETED;
			break;

		case FSM_ORIGINALLOCATION:
			return IDM_SORTBY_ORIGINALLOCATION;
			break;

		case FSM_ATTRIBUTES:
			return IDM_SORTBY_ATTRIBUTES;
			break;

		case FSM_REALSIZE:
			return IDM_SORTBY_REALSIZE;
			break;

		case FSM_SHORTNAME:
			return IDM_SORTBY_SHORTNAME;
			break;

		case FSM_OWNER:
			return IDM_SORTBY_OWNER;
			break;

		case FSM_PRODUCTNAME:
			return IDM_SORTBY_PRODUCTNAME;
			break;

		case FSM_COMPANY:
			return IDM_SORTBY_COMPANY;
			break;

		case FSM_DESCRIPTION:
			return IDM_SORTBY_DESCRIPTION;
			break;

		case FSM_FILEVERSION:
			return IDM_SORTBY_FILEVERSION;
			break;

		case FSM_PRODUCTVERSION:
			return IDM_SORTBY_PRODUCTVERSION;
			break;

		case FSM_SHORTCUTTO:
			return IDM_SORTBY_SHORTCUTTO;
			break;

		case FSM_HARDLINKS:
			return IDM_SORTBY_HARDLINKS;
			break;

		case FSM_EXTENSION:
			return IDM_SORTBY_EXTENSION;
			break;

		case FSM_CREATED:
			return IDM_SORTBY_CREATED;
			break;

		case FSM_ACCESSED:
			return IDM_SORTBY_ACCESSED;
			break;

		case FSM_TITLE:
			return IDM_SORTBY_TITLE;
			break;

		case FSM_SUBJECT:
			return IDM_SORTBY_SUBJECT;
			break;

		case FSM_AUTHOR:
			return IDM_SORTBY_AUTHOR;
			break;

		case FSM_KEYWORDS:
			return IDM_SORTBY_KEYWORDS;
			break;

		case FSM_CAMERAMODEL:
			return IDM_SORTBY_CAMERAMODEL;
			break;

		case FSM_DATETAKEN:
			return IDM_SORTBY_DATETAKEN;
			break;

		case FSM_WIDTH:
			return IDM_SORTBY_WIDTH;
			break;

		case FSM_HEIGHT:
			return IDM_SORTBY_HEIGHT;
			break;

		case FSM_VIRTUALCOMMENTS:
			return IDM_SORTBY_VIRTUALCOMMENTS;
			break;

		case FSM_FILESYSTEM:
			return IDM_SORTBY_FILESYSTEM;
			break;

		case FSM_VIRTUALTYPE:
			return IDM_SORTBY_VIRTUALTYPE;
			break;

		case FSM_NUMPRINTERDOCUMENTS:
			return IDM_SORTBY_NUMPRINTERDOCUMENTS;
			break;

		case FSM_PRINTERSTATUS:
			return IDM_SORTBY_PRINTERSTATUS;
			break;

		case FSM_PRINTERCOMMENTS:
			return IDM_SORTBY_PRINTERCOMMENTS;
			break;

		case FSM_PRINTERLOCATION:
			return IDM_SORTBY_PRINTERLOCATION;
			break;

		case FSM_NETWORKADAPTER_STATUS:
			return IDM_SORTBY_NETWORKADAPTER_STATUS;
			break;
	}

	return -1;
}

int Explorerplusplus::DetermineGroupModeMenuId(int SortMode)
{
	switch(SortMode)
	{
		case FSM_NAME:
			return IDM_GROUPBY_NAME;
			break;

		case FSM_DATEMODIFIED:
			return IDM_GROUPBY_DATEMODIFIED;
			break;

		case FSM_SIZE:
			return IDM_GROUPBY_SIZE;
			break;

		case FSM_TYPE:
			return IDM_GROUPBY_TYPE;
			break;

		case FSM_TOTALSIZE:
			return IDM_GROUPBY_TOTALSIZE;
			break;

		case FSM_FREESPACE:
			return IDM_GROUPBY_FREESPACE;
			break;

		case FSM_COMMENTS:
			return IDM_GROUPBY_COMMENTS;
			break;

		case FSM_DATEDELETED:
			return IDM_GROUPBY_DATEDELETED;
			break;

		case FSM_ORIGINALLOCATION:
			return IDM_GROUPBY_ORIGINALLOCATION;
			break;

		case FSM_ATTRIBUTES:
			return IDM_GROUPBY_ATTRIBUTES;
			break;

		case FSM_REALSIZE:
			return IDM_GROUPBY_REALSIZE;
			break;

		case FSM_SHORTNAME:
			return IDM_GROUPBY_SHORTNAME;
			break;

		case FSM_OWNER:
			return IDM_GROUPBY_OWNER;
			break;

		case FSM_PRODUCTNAME:
			return IDM_GROUPBY_PRODUCTNAME;
			break;

		case FSM_COMPANY:
			return IDM_GROUPBY_COMPANY;
			break;

		case FSM_DESCRIPTION:
			return IDM_GROUPBY_DESCRIPTION;
			break;

		case FSM_FILEVERSION:
			return IDM_GROUPBY_FILEVERSION;
			break;

		case FSM_PRODUCTVERSION:
			return IDM_GROUPBY_PRODUCTVERSION;
			break;

		case FSM_SHORTCUTTO:
			return IDM_GROUPBY_SHORTCUTTO;
			break;

		case FSM_HARDLINKS:
			return IDM_GROUPBY_HARDLINKS;
			break;

		case FSM_EXTENSION:
			return IDM_GROUPBY_EXTENSION;
			break;

		case FSM_CREATED:
			return IDM_GROUPBY_CREATED;
			break;

		case FSM_ACCESSED:
			return IDM_GROUPBY_ACCESSED;
			break;

		case FSM_TITLE:
			return IDM_GROUPBY_TITLE;
			break;

		case FSM_SUBJECT:
			return IDM_GROUPBY_SUBJECT;
			break;

		case FSM_AUTHOR:
			return IDM_GROUPBY_AUTHOR;
			break;

		case FSM_KEYWORDS:
			return IDM_GROUPBY_KEYWORDS;
			break;

		case FSM_CAMERAMODEL:
			return IDM_GROUPBY_CAMERAMODEL;
			break;

		case FSM_DATETAKEN:
			return IDM_GROUPBY_DATETAKEN;
			break;

		case FSM_WIDTH:
			return IDM_GROUPBY_WIDTH;
			break;

		case FSM_HEIGHT:
			return IDM_GROUPBY_HEIGHT;
			break;

		case FSM_VIRTUALCOMMENTS:
			return IDM_GROUPBY_VIRTUALCOMMENTS;
			break;

		case FSM_FILESYSTEM:
			return IDM_GROUPBY_FILESYSTEM;
			break;

		case FSM_VIRTUALTYPE:
			return IDM_GROUPBY_VIRTUALTYPE;
			break;

		case FSM_NUMPRINTERDOCUMENTS:
			return IDM_GROUPBY_NUMPRINTERDOCUMENTS;
			break;

		case FSM_PRINTERSTATUS:
			return IDM_GROUPBY_PRINTERSTATUS;
			break;

		case FSM_PRINTERCOMMENTS:
			return IDM_GROUPBY_PRINTERCOMMENTS;
			break;

		case FSM_PRINTERLOCATION:
			return IDM_GROUPBY_PRINTERLOCATION;
			break;

		case FSM_NETWORKADAPTER_STATUS:
			return IDM_GROUPBY_NETWORKADAPTER_STATUS;
			break;
	}

	return -1;
}

/*
 * Removes the previous arrange menu items from the menu.
 */
void Explorerplusplus::DeletePreviousArrangeMenuItems(void)
{
	MENUITEMINFO	mii;
	int				i = 0;

	for(i = m_iMaxArrangeMenuItem - 1;i >= 0;i--)
	{
		mii.cbSize	= sizeof(mii);
		mii.fMask	= MIIM_DATA;
		GetMenuItemInfo(m_hArrangeSubMenu,i,TRUE,&mii);

		free((CustomMenuInfo_t *)mii.dwItemData);

		mii.cbSize	= sizeof(mii);
		mii.fMask	= MIIM_DATA;
		GetMenuItemInfo(m_hGroupBySubMenu,i,TRUE,&mii);

		free((CustomMenuInfo_t *)mii.dwItemData);

		DeleteMenu(m_hArrangeSubMenu,i,MF_BYPOSITION);
		DeleteMenu(m_hGroupBySubMenu,i,MF_BYPOSITION);
	}
}

/*
 * Updates the arrange menu with the new items.
 */
void Explorerplusplus::UpdateArrangeMenuItems(void)
{
	list<int>				SortModes;
	list<int>::iterator		itr;
	ArrangeMenuItem_t		am;
	int						SortById;
	int						GroupById;

	DeletePreviousArrangeMenuItems();

	m_pActiveShellBrowser->QueryCurrentSortModes(&SortModes);

	m_ArrangeList.clear();

	if(SortModes.size() != 0)
	{
		for(itr = SortModes.begin();itr != SortModes.end();itr++)
		{
			SortById = DetermineSortModeMenuId(*itr);
			GroupById = DetermineGroupModeMenuId(*itr);

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

	/* Do NOT mark the whole menu to be owner
	drawn. The lower menus (below the sort modes)
	are already owner drawn. ONLY mark the current
	sort modes as owner drawn. */
	SetMenuOwnerDrawInternal(m_hArrangeSubMenu,m_iMaxArrangeMenuItem);
	SetMenuOwnerDrawInternal(m_hGroupBySubMenu,m_iMaxArrangeMenuItem);
}