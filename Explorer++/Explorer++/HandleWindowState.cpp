/******************************************************************
 *
 * Project: Explorer++
 * File: HandleWindowState.cpp
 * License: GPL - See LICENSE in the top level directory
 *
 * Keeps track of the state of the main window
 * and its child windows.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include <shobjidl.h>
#include "Explorer++.h"
#include "MainResource.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/Controls.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/TabHelper.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/Macros.h"


/* Tab icons. */
#define TAB_ICON_LOCK_INDEX			0

#define SORTBY_BASE	50000
#define SORTBY_END	50099

#define GROUPBY_BASE	50100
#define GROUPBY_END		50199

void Explorerplusplus::UpdateWindowStates(void)
{
	m_pActiveShellBrowser->QueryCurrentDirectory(SIZEOF_ARRAY(m_CurrentDirectory),m_CurrentDirectory);

	UpdateMainWindowText();
	UpdateAddressBarText();
	UpdateMainToolbar();
	UpdateTabText();
	UpdateTreeViewSelection();
	UpdateStatusBarText();
	UpdateTabToolbar();
	UpdateDisplayWindow();

	if(m_bShowFolders)
		SendMessage(m_hMainToolbar,TB_CHECKBUTTON,(WPARAM)TOOLBAR_FOLDERS,(LPARAM)TRUE);
}

/*
* Set the state of the items in the main
* program menu.
*/
void Explorerplusplus::SetProgramMenuItemStates(HMENU hProgramMenu)
{
	UINT uViewMode = m_pActiveShellBrowser->GetCurrentViewMode();
	BOOL bVirtualFolder = m_pActiveShellBrowser->InVirtualFolder();

	lEnableMenuItem(hProgramMenu,IDM_FILE_COPYITEMPATH,AnyItemsSelected());
	lEnableMenuItem(hProgramMenu,IDM_FILE_COPYUNIVERSALFILEPATHS,AnyItemsSelected());
	lEnableMenuItem(hProgramMenu,IDM_FILE_SETFILEATTRIBUTES,AnyItemsSelected());
	lEnableMenuItem(hProgramMenu,IDM_FILE_OPENCOMMANDPROMPT,!bVirtualFolder);
	lEnableMenuItem(hProgramMenu,IDM_FILE_SAVEDIRECTORYLISTING,!bVirtualFolder);
	lEnableMenuItem(hProgramMenu,IDM_FILE_COPYCOLUMNTEXT,m_nSelected && (uViewMode == VM_DETAILS));

	lEnableMenuItem(hProgramMenu,IDM_FILE_RENAME,IsRenamePossible());
	lEnableMenuItem(hProgramMenu,IDM_FILE_DELETE,IsDeletionPossible());
	lEnableMenuItem(hProgramMenu,IDM_FILE_DELETEPERMANENTLY,IsDeletionPossible());
	lEnableMenuItem(hProgramMenu,IDM_FILE_PROPERTIES,CanShowFileProperties());

	lEnableMenuItem(hProgramMenu,IDM_EDIT_UNDO,m_FileActionHandler.CanUndo());
	lEnableMenuItem(hProgramMenu,IDM_EDIT_PASTE,CanPaste());
	lEnableMenuItem(hProgramMenu,IDM_EDIT_PASTESHORTCUT,CanPaste());
	lEnableMenuItem(hProgramMenu,IDM_EDIT_PASTEHARDLINK,CanPaste());

	/* The following menu items are only enabled when one
	or more files are selected (they represent file
	actions, cut/copy, etc). */
	/* TODO: Split CanCutOrCopySelection() into two, as some
	items may only be copied/cut (not both). */
	lEnableMenuItem(hProgramMenu,IDM_EDIT_COPY,CanCutOrCopySelection());
	lEnableMenuItem(hProgramMenu,IDM_EDIT_CUT,CanCutOrCopySelection());
	lEnableMenuItem(hProgramMenu,IDM_EDIT_COPYTOFOLDER,CanCutOrCopySelection() && GetFocus() != m_hTreeView);
	lEnableMenuItem(hProgramMenu,IDM_EDIT_MOVETOFOLDER,CanCutOrCopySelection() && GetFocus() != m_hTreeView);
	lEnableMenuItem(hProgramMenu,IDM_EDIT_WILDCARDDESELECT,m_nSelected);
	lEnableMenuItem(hProgramMenu,IDM_EDIT_SELECTNONE,m_nSelected);
	lEnableMenuItem(hProgramMenu,IDM_EDIT_RESOLVELINK,m_nSelected);

	lCheckMenuItem(hProgramMenu,IDM_VIEW_STATUSBAR,m_bShowStatusBar);
	lCheckMenuItem(hProgramMenu,IDM_VIEW_FOLDERS,m_bShowFolders);
	lCheckMenuItem(hProgramMenu,IDM_VIEW_DISPLAYWINDOW,m_bShowDisplayWindow);
	lCheckMenuItem(hProgramMenu,IDM_TOOLBARS_ADDRESSBAR,m_bShowAddressBar);
	lCheckMenuItem(hProgramMenu,IDM_TOOLBARS_MAINTOOLBAR,m_bShowMainToolbar);
	lCheckMenuItem(hProgramMenu,IDM_TOOLBARS_BOOKMARKSTOOLBAR,m_bShowBookmarksToolbar);
	lCheckMenuItem(hProgramMenu,IDM_TOOLBARS_DRIVES,m_bShowDrivesToolbar);
	lCheckMenuItem(hProgramMenu,IDM_TOOLBARS_APPLICATIONTOOLBAR,m_bShowApplicationToolbar);
	lCheckMenuItem(hProgramMenu,IDM_TOOLBARS_LOCKTOOLBARS,m_bLockToolbars);
	lCheckMenuItem(hProgramMenu,IDM_VIEW_SHOWHIDDENFILES,m_pActiveShellBrowser->GetShowHidden());
	lCheckMenuItem(hProgramMenu,IDM_FILTER_APPLYFILTER,m_pActiveShellBrowser->GetFilterStatus());

	lEnableMenuItem(hProgramMenu,IDM_ACTIONS_NEWFOLDER,!bVirtualFolder);
	lEnableMenuItem(hProgramMenu,IDM_ACTIONS_SPLITFILE,(m_pActiveShellBrowser->QueryNumSelectedFiles() == 1) && !bVirtualFolder);
	lEnableMenuItem(hProgramMenu,IDM_ACTIONS_MERGEFILES,m_nSelected > 1);
	lEnableMenuItem(hProgramMenu,IDM_ACTIONS_DESTROYFILES,m_nSelected);

	UINT ItemToCheck = GetViewModeMenuId(uViewMode);
	CheckMenuRadioItem(hProgramMenu,IDM_VIEW_THUMBNAILS,IDM_VIEW_EXTRALARGEICONS,ItemToCheck,MF_BYCOMMAND);

	lEnableMenuItem(hProgramMenu,IDM_FILE_CLOSETAB,TabCtrl_GetItemCount(m_hTabCtrl));
	lEnableMenuItem(hProgramMenu,IDM_GO_BACK,m_pActiveShellBrowser->IsBackHistory());
	lEnableMenuItem(hProgramMenu,IDM_GO_FORWARD,m_pActiveShellBrowser->IsForwardHistory());
	lEnableMenuItem(hProgramMenu,IDM_GO_UPONELEVEL,m_pActiveShellBrowser->CanBrowseUp());

	lEnableMenuItem(hProgramMenu,IDM_VIEW_AUTOSIZECOLUMNS,uViewMode == VM_DETAILS);

	if(uViewMode == VM_DETAILS)
	{
		/* Disable auto arrange menu item. */
		lEnableMenuItem(hProgramMenu,IDM_ARRANGEICONSBY_AUTOARRANGE,FALSE);
		lCheckMenuItem(hProgramMenu,IDM_ARRANGEICONSBY_AUTOARRANGE,FALSE);

		lEnableMenuItem(hProgramMenu,IDM_VIEW_GROUPBY,TRUE);
	}
	else if(uViewMode == VM_LIST)
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
	UINT SortMode;
	BOOL bShowInGroups;
	BOOL bVirtualFolder;
	UINT uFirst;
	UINT uLast;
	HMENU hMenu;
	HMENU hMenuRClick;
	int nItems;
	int i = 0;

	bVirtualFolder = m_pActiveShellBrowser->InVirtualFolder();

	SortMode = m_pActiveShellBrowser->GetSortMode();

	bShowInGroups = m_pActiveShellBrowser->IsGroupViewEnabled();

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

		ItemToCheck = DetermineGroupModeMenuId(SortMode);

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

		ItemToCheck = DetermineSortModeMenuId(SortMode);

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
	if(m_bShowFullTitlePath && !m_pActiveShellBrowser->InVirtualFolder())
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

	if(m_bShowUserNameInTitleBar || m_bShowPrivilegeLevelInTitleBar)
		StringCchCat(szTitle,SIZEOF_ARRAY(szTitle),_T(" ["));

	if(m_bShowUserNameInTitleBar)
	{
		GetProcessOwner(GetCurrentProcessId(),szOwner,SIZEOF_ARRAY(szOwner));

		StringCchCat(szTitle,SIZEOF_ARRAY(szTitle),szOwner);
	}

	if(m_bShowPrivilegeLevelInTitleBar)
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

		if(m_bShowUserNameInTitleBar)
			StringCchPrintf(szPrivilegeAddition,SIZEOF_ARRAY(szPrivilegeAddition),
			_T(" - %s"),szPrivilege);
		else
			StringCchPrintf(szPrivilegeAddition,SIZEOF_ARRAY(szPrivilegeAddition),
			_T("%s"),szPrivilege);

		StringCchCat(szTitle,SIZEOF_ARRAY(szTitle),szPrivilegeAddition);
	}

	if(m_bShowUserNameInTitleBar || m_bShowPrivilegeLevelInTitleBar)
		StringCchCat(szTitle,SIZEOF_ARRAY(szTitle),_T("]"));

	SetWindowText(m_hContainer,szTitle);
}

void Explorerplusplus::UpdateAddressBarText(void)
{
	LPITEMIDLIST pidl = NULL;
	TCHAR szAddressBarTitle[MAX_PATH];

	pidl = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

	TCHAR szParsingPath[MAX_PATH];

	GetDisplayName(pidl,szParsingPath,SIZEOF_ARRAY(szParsingPath),SHGDN_FORPARSING);

	/* If the path is a GUID (i.e. of the form
	::{20D04FE0-3AEA-1069-A2D8-08002B30309D}), we'll
	switch to showing the in folder name.
	Otherwise, we'll show the full path.
	Driven by the principle that GUID's should NOT
	be shown directly to users. */
	if(IsPathGUID(szParsingPath))
	{
		GetDisplayName(pidl,szAddressBarTitle,SIZEOF_ARRAY(szAddressBarTitle),SHGDN_INFOLDER);
	}
	else
	{
		StringCchCopy(szAddressBarTitle,SIZEOF_ARRAY(szAddressBarTitle),
			szParsingPath);
	}

	SetAddressBarText(pidl,szAddressBarTitle);

	CoTaskMemFree(pidl);
}

void Explorerplusplus::UpdateTabText(void)
{
	UpdateTabText(m_selectedTabIndex,m_selectedTabId);
}

void Explorerplusplus::UpdateTabText(int iTabId)
{
	TCITEM tcItem;
	int nTabs;
	int i = 0;

	nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	for(i = 0;i < nTabs;i++)
	{
		tcItem.mask = TCIF_PARAM;
		TabCtrl_GetItem(m_hTabCtrl,i,&tcItem);

		if((int)tcItem.lParam == iTabId)
		{
			UpdateTabText(i,iTabId);
			break;
		}
	}
}

void Explorerplusplus::UpdateTabText(int iTab,int iTabId)
{
	TCHAR szFinalTabText[MAX_PATH];

	if(!m_TabInfo.at(iTabId).bUseCustomName)
	{
		LPITEMIDLIST pidlDirectory = m_pShellBrowser[iTabId]->QueryCurrentDirectoryIdl();

		TCHAR szTabText[MAX_PATH];
		GetDisplayName(pidlDirectory,szTabText,SIZEOF_ARRAY(szTabText),SHGDN_INFOLDER);

		StringCchCopy(m_TabInfo.at(iTabId).szName,
			SIZEOF_ARRAY(m_TabInfo.at(iTabId).szName),szTabText);

		TCHAR szExpandedTabText[MAX_PATH];
		ReplaceCharacterWithString(szTabText,szExpandedTabText,
			SIZEOF_ARRAY(szExpandedTabText),'&',_T("&&"));

		TabCtrl_SetItemText(m_hTabCtrl,iTab,szExpandedTabText);
		StringCchCopy(szFinalTabText,SIZEOF_ARRAY(szFinalTabText),szExpandedTabText);

		CoTaskMemFree(pidlDirectory);
	}
	else
	{
		StringCchCopy(szFinalTabText,SIZEOF_ARRAY(szFinalTabText),m_TabInfo.at(iTabId).szName);
	}

	/* Set the tab proxy text. */
	UpdateTaskbarThumbnailTtitle(iTabId, szFinalTabText);
}

void Explorerplusplus::SetTabIcon(void)
{
	LPITEMIDLIST pidl = NULL;

	pidl = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

	SetTabIcon(m_selectedTabIndex,m_selectedTabId,pidl);

	CoTaskMemFree(pidl);
}

void Explorerplusplus::SetTabIcon(int iTabId)
{
	LPITEMIDLIST pidl = NULL;
	TCITEM tcItem;
	int nTabs;
	int i = 0;

	pidl = m_pShellBrowser[iTabId]->QueryCurrentDirectoryIdl();

	nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	for(i = 0;i < nTabs;i++)
	{
		tcItem.mask = TCIF_PARAM;
		TabCtrl_GetItem(m_hTabCtrl,i,&tcItem);

		if((int)tcItem.lParam == iTabId)
		{
			SetTabIcon(i,iTabId,pidl);
			break;
		}
	}

	CoTaskMemFree(pidl);
}

void Explorerplusplus::SetTabIcon(int iIndex,int iTabId)
{
	LPITEMIDLIST pidl = NULL;

	pidl = m_pShellBrowser[iTabId]->QueryCurrentDirectoryIdl();

	SetTabIcon(iIndex,iTabId,pidl);

	CoTaskMemFree(pidl);
}

/* Sets a tabs icon. Normally, this icon
is the folders icon, however if the tab
is locked, the icon will be a lock. */
void Explorerplusplus::SetTabIcon(int iIndex,int iTabId,LPCITEMIDLIST pidlDirectory)
{
	TCITEM			tcItem;
	SHFILEINFO		shfi;
	ICONINFO		IconInfo;
	int				iImage;
	int				iRemoveImage;

	/* If the tab is locked, use a lock icon. */
	if(m_TabInfo.at(iTabId).bAddressLocked || m_TabInfo.at(iTabId).bLocked)
	{
		iImage = TAB_ICON_LOCK_INDEX;
	}
	else
	{
		SHGetFileInfo((LPCTSTR)pidlDirectory,0,&shfi,sizeof(shfi),
			SHGFI_PIDL|SHGFI_ICON|SHGFI_SMALLICON);

		/* TODO: The proxy icon may also be the lock icon, if
		the tab is locked. */
		SetTabProxyIcon(iTabId,shfi.hIcon);

		GetIconInfo(shfi.hIcon,&IconInfo);
		iImage = ImageList_Add(TabCtrl_GetImageList(m_hTabCtrl),
			IconInfo.hbmColor,IconInfo.hbmMask);

		DeleteObject(IconInfo.hbmColor);
		DeleteObject(IconInfo.hbmMask);
		DestroyIcon(shfi.hIcon);
	}

	/* Get the index of the current image. This image
	will be removed after the new image is set. */
	tcItem.mask		= TCIF_IMAGE;
	TabCtrl_GetItem(m_hTabCtrl,iIndex,&tcItem);

	iRemoveImage = tcItem.iImage;

	/* Set the new image. */
	tcItem.mask		= TCIF_IMAGE;
	tcItem.iImage	= iImage;
	TabCtrl_SetItem(m_hTabCtrl,iIndex,&tcItem);

	if(iRemoveImage != TAB_ICON_LOCK_INDEX)
	{
		/* Remove the old image. */
		TabCtrl_RemoveImage(m_hTabCtrl,iRemoveImage);
	}
}