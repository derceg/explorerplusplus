// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "ShellBrowser/ViewModes.h"
#include "SortModeHelper.h"
#include "ToolbarButtons.h"
#include "../Helper/Controls.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/TabHelper.h"
#include <shobjidl.h>
#include <list>

#define SORTBY_BASE	50000
#define SORTBY_END	50099

#define GROUPBY_BASE	50100
#define GROUPBY_END		50199

void Explorerplusplus::UpdateWindowStates(void)
{
	m_pActiveShellBrowser->QueryCurrentDirectory(SIZEOF_ARRAY(m_CurrentDirectory),m_CurrentDirectory);

	UpdateMainWindowText();
	UpdateTreeViewSelection();
	UpdateStatusBarText();
	UpdateTabToolbar();
	UpdateDisplayWindow();
}

/*
* Set the state of the items in the main
* program menu.
*/
void Explorerplusplus::SetProgramMenuItemStates(HMENU hProgramMenu)
{
	ViewMode viewMode = m_pActiveShellBrowser->GetViewMode();
	BOOL bVirtualFolder = m_pActiveShellBrowser->InVirtualFolder();

	lEnableMenuItem(hProgramMenu,IDM_FILE_COPYITEMPATH,AnyItemsSelected());
	lEnableMenuItem(hProgramMenu,IDM_FILE_COPYUNIVERSALFILEPATHS,AnyItemsSelected());
	lEnableMenuItem(hProgramMenu,IDM_FILE_SETFILEATTRIBUTES,AnyItemsSelected());
	lEnableMenuItem(hProgramMenu,IDM_FILE_OPENCOMMANDPROMPT,!bVirtualFolder);
	lEnableMenuItem(hProgramMenu,IDM_FILE_SAVEDIRECTORYLISTING,!bVirtualFolder);
	lEnableMenuItem(hProgramMenu,IDM_FILE_COPYCOLUMNTEXT,m_nSelected && (viewMode == +ViewMode::Details));

	lEnableMenuItem(hProgramMenu,IDM_FILE_RENAME,CanRename());
	lEnableMenuItem(hProgramMenu,IDM_FILE_DELETE,CanDelete());
	lEnableMenuItem(hProgramMenu,IDM_FILE_DELETEPERMANENTLY,CanDelete());
	lEnableMenuItem(hProgramMenu,IDM_FILE_PROPERTIES,CanShowFileProperties());

	lEnableMenuItem(hProgramMenu,IDM_EDIT_UNDO,m_FileActionHandler.CanUndo());
	lEnableMenuItem(hProgramMenu,IDM_EDIT_PASTE,CanPaste());
	lEnableMenuItem(hProgramMenu,IDM_EDIT_PASTESHORTCUT,CanPaste());
	lEnableMenuItem(hProgramMenu,IDM_EDIT_PASTEHARDLINK,CanPaste());

	/* The following menu items are only enabled when one
	or more files are selected (they represent file
	actions, cut/copy, etc). */
	lEnableMenuItem(hProgramMenu,IDM_EDIT_COPY,CanCopy());
	lEnableMenuItem(hProgramMenu,IDM_EDIT_CUT,CanCut());
	lEnableMenuItem(hProgramMenu,IDM_EDIT_COPYTOFOLDER,CanCopy() && GetFocus() != m_hTreeView);
	lEnableMenuItem(hProgramMenu,IDM_EDIT_MOVETOFOLDER,CanCut() && GetFocus() != m_hTreeView);
	lEnableMenuItem(hProgramMenu,IDM_EDIT_WILDCARDDESELECT,m_nSelected);
	lEnableMenuItem(hProgramMenu,IDM_EDIT_SELECTNONE,m_nSelected);
	lEnableMenuItem(hProgramMenu,IDM_EDIT_RESOLVELINK,m_nSelected);

	lCheckMenuItem(hProgramMenu,IDM_VIEW_STATUSBAR,m_config->showStatusBar);
	lCheckMenuItem(hProgramMenu,IDM_VIEW_FOLDERS, m_config->showFolders);
	lCheckMenuItem(hProgramMenu,IDM_VIEW_DISPLAYWINDOW,m_config->showDisplayWindow);
	lCheckMenuItem(hProgramMenu,IDM_TOOLBARS_ADDRESSBAR, m_config->showAddressBar);
	lCheckMenuItem(hProgramMenu,IDM_TOOLBARS_MAINTOOLBAR,m_config->showMainToolbar);
	lCheckMenuItem(hProgramMenu,IDM_TOOLBARS_BOOKMARKSTOOLBAR,m_config->showBookmarksToolbar);
	lCheckMenuItem(hProgramMenu,IDM_TOOLBARS_DRIVES,m_config->showDrivesToolbar);
	lCheckMenuItem(hProgramMenu,IDM_TOOLBARS_APPLICATIONTOOLBAR,m_config->showApplicationToolbar);
	lCheckMenuItem(hProgramMenu,IDM_TOOLBARS_LOCKTOOLBARS,m_config->lockToolbars);
	lCheckMenuItem(hProgramMenu,IDM_VIEW_SHOWHIDDENFILES,m_pActiveShellBrowser->GetShowHidden());
	lCheckMenuItem(hProgramMenu,IDM_FILTER_APPLYFILTER,m_pActiveShellBrowser->GetFilterStatus());

	lEnableMenuItem(hProgramMenu,IDM_ACTIONS_NEWFOLDER,CanCreate());
	lEnableMenuItem(hProgramMenu,IDM_ACTIONS_SPLITFILE,(m_pActiveShellBrowser->GetNumSelectedFiles() == 1) && !bVirtualFolder);
	lEnableMenuItem(hProgramMenu,IDM_ACTIONS_MERGEFILES,m_nSelected > 1);
	lEnableMenuItem(hProgramMenu,IDM_ACTIONS_DESTROYFILES,m_nSelected);

	UINT ItemToCheck = GetViewModeMenuId(viewMode);
	CheckMenuRadioItem(hProgramMenu,IDM_VIEW_THUMBNAILS,IDM_VIEW_EXTRALARGEICONS,ItemToCheck,MF_BYCOMMAND);

	lEnableMenuItem(hProgramMenu,IDM_GO_BACK,m_pActiveShellBrowser->CanBrowseBack());
	lEnableMenuItem(hProgramMenu,IDM_GO_FORWARD,m_pActiveShellBrowser->CanBrowseForward());
	lEnableMenuItem(hProgramMenu,IDM_GO_UPONELEVEL,m_pActiveShellBrowser->CanBrowseUp());

	lEnableMenuItem(hProgramMenu,IDM_VIEW_AUTOSIZECOLUMNS,viewMode == +ViewMode::Details);

	if(viewMode == +ViewMode::Details)
	{
		/* Disable auto arrange menu item. */
		lEnableMenuItem(hProgramMenu,IDM_ARRANGEICONSBY_AUTOARRANGE,FALSE);
		lCheckMenuItem(hProgramMenu,IDM_ARRANGEICONSBY_AUTOARRANGE,FALSE);

		lEnableMenuItem(hProgramMenu,IDM_VIEW_GROUPBY,TRUE);
	}
	else if(viewMode == +ViewMode::List)
	{
		/* Disable group menu item. */
		lEnableMenuItem(hProgramMenu,IDM_VIEW_GROUPBY,FALSE);

		/* Disable auto arrange menu item. */
		lEnableMenuItem(hProgramMenu,IDM_ARRANGEICONSBY_AUTOARRANGE,FALSE);
		lCheckMenuItem(hProgramMenu,IDM_ARRANGEICONSBY_AUTOARRANGE,FALSE);
	}
	else
	{
		lEnableMenuItem(hProgramMenu,IDM_VIEW_GROUPBY,TRUE);

		lEnableMenuItem(hProgramMenu,IDM_ARRANGEICONSBY_AUTOARRANGE,TRUE);
		lCheckMenuItem(hProgramMenu,IDM_ARRANGEICONSBY_AUTOARRANGE,m_pActiveShellBrowser->GetAutoArrange());
	}

	SetArrangeMenuItemStates();
}

/*
* Set the state of the items in the
* 'arrange menu', which appears as a
* submenu in other higher level menus.
*/
void Explorerplusplus::SetArrangeMenuItemStates()
{
	UINT ItemToCheck;
	BOOL bShowInGroups;
	BOOL bVirtualFolder;
	UINT uFirst;
	UINT uLast;
	HMENU hMenu;
	HMENU hMenuRClick;
	int nItems;
	int i = 0;

	bVirtualFolder = m_pActiveShellBrowser->InVirtualFolder();

	const SortMode sortMode = m_pActiveShellBrowser->GetSortMode();

	bShowInGroups = m_pActiveShellBrowser->GetShowInGroups();

	/* Go through both the sort by and group by menus and
	remove all the checkmarks. Alternatively, could remember
	which items have checkmarks, and just uncheck those. */
	nItems = GetMenuItemCount(m_hArrangeSubMenu);

	for(i = 0;i < nItems;i++)
	{
		CheckMenuItem(m_hArrangeSubMenu,i,MF_BYPOSITION|MF_UNCHECKED);
		CheckMenuItem(m_hArrangeSubMenuRClick,i,MF_BYPOSITION|MF_UNCHECKED);
	}

	nItems = GetMenuItemCount(m_hGroupBySubMenu);

	for(i = 0;i < nItems;i++)
	{
		CheckMenuItem(m_hGroupBySubMenu,i,MF_BYPOSITION|MF_UNCHECKED);
		CheckMenuItem(m_hGroupBySubMenuRClick,i,MF_BYPOSITION|MF_UNCHECKED);
	}

	if(bShowInGroups)
	{
		hMenu = m_hGroupBySubMenu;
		hMenuRClick = m_hGroupBySubMenuRClick;

		ItemToCheck = DetermineGroupModeMenuId(sortMode);

		if(ItemToCheck == -1)
		{
			/* Sort mode is invalid. Set it back to the default
			(i.e. sort by name). */
			ItemToCheck = IDM_GROUPBY_NAME;
		}

		uFirst = GROUPBY_BASE;
		uLast = GROUPBY_END;

		lEnableMenuItem(m_hArrangeSubMenu,IDM_ARRANGEICONSBY_ASCENDING,FALSE);
		lEnableMenuItem(m_hArrangeSubMenu,IDM_ARRANGEICONSBY_DESCENDING,FALSE);
		lEnableMenuItem(m_hArrangeSubMenuRClick,IDM_ARRANGEICONSBY_ASCENDING,FALSE);
		lEnableMenuItem(m_hArrangeSubMenuRClick,IDM_ARRANGEICONSBY_DESCENDING,FALSE);

		lEnableMenuItem(m_hGroupBySubMenu,IDM_ARRANGEICONSBY_ASCENDING,TRUE);
		lEnableMenuItem(m_hGroupBySubMenu,IDM_ARRANGEICONSBY_DESCENDING,TRUE);
		lEnableMenuItem(m_hGroupBySubMenuRClick,IDM_ARRANGEICONSBY_ASCENDING,TRUE);
		lEnableMenuItem(m_hGroupBySubMenuRClick,IDM_ARRANGEICONSBY_DESCENDING,TRUE);

		/* May need to change this (i.e. uncheck each menu item
		individually). */
		CheckMenuRadioItem(m_hArrangeSubMenu,SORTBY_BASE,SORTBY_END,
			0,MF_BYCOMMAND);
		CheckMenuRadioItem(m_hArrangeSubMenuRClick,SORTBY_BASE,SORTBY_END,
			0,MF_BYCOMMAND);
	}
	else
	{
		hMenu = m_hArrangeSubMenu;
		hMenuRClick = m_hArrangeSubMenuRClick;

		ItemToCheck = DetermineSortModeMenuId(sortMode);

		if(ItemToCheck == -1)
		{
			/* Sort mode is invalid. Set it back to the default
			(i.e. sort by name). */
			ItemToCheck = IDM_SORTBY_NAME;
		}

		uFirst = SORTBY_BASE;
		uLast = SORTBY_END;

		lEnableMenuItem(m_hGroupBySubMenu,IDM_ARRANGEICONSBY_ASCENDING,FALSE);
		lEnableMenuItem(m_hGroupBySubMenu,IDM_ARRANGEICONSBY_DESCENDING,FALSE);
		lEnableMenuItem(m_hGroupBySubMenuRClick,IDM_ARRANGEICONSBY_ASCENDING,FALSE);
		lEnableMenuItem(m_hGroupBySubMenuRClick,IDM_ARRANGEICONSBY_DESCENDING,FALSE);

		lEnableMenuItem(m_hArrangeSubMenu,IDM_ARRANGEICONSBY_ASCENDING,TRUE);
		lEnableMenuItem(m_hArrangeSubMenu,IDM_ARRANGEICONSBY_DESCENDING,TRUE);
		lEnableMenuItem(m_hArrangeSubMenuRClick,IDM_ARRANGEICONSBY_ASCENDING,TRUE);
		lEnableMenuItem(m_hArrangeSubMenuRClick,IDM_ARRANGEICONSBY_DESCENDING,TRUE);
	}

	CheckMenuRadioItem(hMenu,uFirst,uLast,
		ItemToCheck,MF_BYCOMMAND);
	CheckMenuRadioItem(hMenuRClick,uFirst,uLast,
		ItemToCheck,MF_BYCOMMAND);

	if(m_pActiveShellBrowser->GetSortAscending())
		ItemToCheck = IDM_ARRANGEICONSBY_ASCENDING;
	else
		ItemToCheck = IDM_ARRANGEICONSBY_DESCENDING;

	CheckMenuRadioItem(hMenu,IDM_ARRANGEICONSBY_ASCENDING,IDM_ARRANGEICONSBY_DESCENDING,
		ItemToCheck,MF_BYCOMMAND);
	CheckMenuRadioItem(hMenuRClick,IDM_ARRANGEICONSBY_ASCENDING,IDM_ARRANGEICONSBY_DESCENDING,
		ItemToCheck,MF_BYCOMMAND);
}

void Explorerplusplus::UpdateMainWindowText(void)
{
	TCHAR	szTitle[512];
	TCHAR	szFolderDisplayName[MAX_PATH];
	TCHAR	szOwner[512];

	/* Don't show full paths for virtual folders (as only the folders
	GUID will be shown). */
	if(m_config->showFullTitlePath && !m_pActiveShellBrowser->InVirtualFolder())
	{
		GetDisplayName(m_CurrentDirectory,szFolderDisplayName,SIZEOF_ARRAY(szFolderDisplayName),SHGDN_FORPARSING);
	}
	else
	{
		GetDisplayName(m_CurrentDirectory,szFolderDisplayName,SIZEOF_ARRAY(szFolderDisplayName),SHGDN_NORMAL);
	}

	TCHAR szTemp[64];
	LoadString(m_hLanguageModule, IDS_MAIN_WINDOW_TITLE, szTemp, SIZEOF_ARRAY(szTemp));
	StringCchPrintf(szTitle,SIZEOF_ARRAY(szTitle),
	szTemp,szFolderDisplayName,NExplorerplusplus::APP_NAME);

	if(m_config->showUserNameInTitleBar || m_config->showPrivilegeLevelInTitleBar)
		StringCchCat(szTitle,SIZEOF_ARRAY(szTitle),_T(" ["));

	if(m_config->showUserNameInTitleBar)
	{
		GetProcessOwner(GetCurrentProcessId(),szOwner,SIZEOF_ARRAY(szOwner));

		StringCchCat(szTitle,SIZEOF_ARRAY(szTitle),szOwner);
	}

	if(m_config->showPrivilegeLevelInTitleBar)
	{
		TCHAR szPrivilegeAddition[64];
		TCHAR szPrivilege[64];

		if(CheckGroupMembership(GROUP_ADMINISTRATORS))
		{
			LoadString(m_hLanguageModule,IDS_PRIVILEGE_LEVEL_ADMINISTRATORS,szPrivilege,SIZEOF_ARRAY(szPrivilege));
		}
		else if(CheckGroupMembership(GROUP_POWERUSERS))
		{
			LoadString(m_hLanguageModule,IDS_PRIVILEGE_LEVEL_POWER_USERS,szPrivilege,SIZEOF_ARRAY(szPrivilege));
		}
		else if(CheckGroupMembership(GROUP_USERS))
		{
			LoadString(m_hLanguageModule,IDS_PRIVILEGE_LEVEL_USERS,szPrivilege,SIZEOF_ARRAY(szPrivilege));
		}
		else if(CheckGroupMembership(GROUP_USERSRESTRICTED))
		{
			LoadString(m_hLanguageModule,IDS_PRIVILEGE_LEVEL_USERS_RESTRICTED,szPrivilege,SIZEOF_ARRAY(szPrivilege));
		}

		if(m_config->showUserNameInTitleBar)
			StringCchPrintf(szPrivilegeAddition,SIZEOF_ARRAY(szPrivilegeAddition),
			_T(" - %s"),szPrivilege);
		else
			StringCchPrintf(szPrivilegeAddition,SIZEOF_ARRAY(szPrivilegeAddition),
			_T("%s"),szPrivilege);

		StringCchCat(szTitle,SIZEOF_ARRAY(szTitle),szPrivilegeAddition);
	}

	if(m_config->showUserNameInTitleBar || m_config->showPrivilegeLevelInTitleBar)
		StringCchCat(szTitle,SIZEOF_ARRAY(szTitle),_T("]"));

	SetWindowText(m_hContainer,szTitle);
}