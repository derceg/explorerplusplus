// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
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

void Explorerplusplus::UpdateWindowStates(const Tab &tab)
{
	m_CurrentDirectory = tab.GetShellBrowser()->GetDirectory();

	UpdateStatusBarText(tab);
	UpdateDisplayWindow(tab);
}

/*
* Set the state of the items in the main
* program menu.
*/
void Explorerplusplus::SetProgramMenuItemStates(HMENU hProgramMenu)
{
	const Tab &tab = m_tabContainer->GetSelectedTab();

	ViewMode viewMode = tab.GetShellBrowser()->GetViewMode();
	BOOL bVirtualFolder = tab.GetShellBrowser()->InVirtualFolder();

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
	lCheckMenuItem(hProgramMenu,IDM_VIEW_SHOWHIDDENFILES,tab.GetShellBrowser()->GetShowHidden());
	lCheckMenuItem(hProgramMenu,IDM_FILTER_APPLYFILTER,tab.GetShellBrowser()->GetFilterStatus());

	lEnableMenuItem(hProgramMenu,IDM_ACTIONS_NEWFOLDER,CanCreate());
	lEnableMenuItem(hProgramMenu,IDM_ACTIONS_SPLITFILE,(tab.GetShellBrowser()->GetNumSelectedFiles() == 1) && !bVirtualFolder);
	lEnableMenuItem(hProgramMenu,IDM_ACTIONS_MERGEFILES,m_nSelected > 1);
	lEnableMenuItem(hProgramMenu,IDM_ACTIONS_DESTROYFILES,m_nSelected);

	UINT ItemToCheck = GetViewModeMenuId(viewMode);
	CheckMenuRadioItem(hProgramMenu,IDM_VIEW_THUMBNAILS,IDM_VIEW_EXTRALARGEICONS,ItemToCheck,MF_BYCOMMAND);

	lEnableMenuItem(hProgramMenu,IDM_GO_BACK,tab.GetNavigationController()->CanGoBack());
	lEnableMenuItem(hProgramMenu,IDM_GO_FORWARD,tab.GetNavigationController()->CanGoForward());
	lEnableMenuItem(hProgramMenu,IDM_GO_UPONELEVEL,tab.GetNavigationController()->CanGoUp());

	lEnableMenuItem(hProgramMenu,IDM_VIEW_AUTOSIZECOLUMNS,viewMode == +ViewMode::Details);

	if(viewMode == +ViewMode::Details)
	{
		/* Disable auto arrange menu item. */
		lEnableMenuItem(hProgramMenu,IDM_VIEW_AUTOARRANGE,FALSE);
		lCheckMenuItem(hProgramMenu,IDM_VIEW_AUTOARRANGE,FALSE);

		lEnableMenuItem(hProgramMenu,IDM_VIEW_GROUPBY,TRUE);
	}
	else if(viewMode == +ViewMode::List)
	{
		/* Disable group menu item. */
		lEnableMenuItem(hProgramMenu,IDM_VIEW_GROUPBY,FALSE);

		/* Disable auto arrange menu item. */
		lEnableMenuItem(hProgramMenu,IDM_VIEW_AUTOARRANGE,FALSE);
		lCheckMenuItem(hProgramMenu,IDM_VIEW_AUTOARRANGE,FALSE);
	}
	else
	{
		lEnableMenuItem(hProgramMenu,IDM_VIEW_GROUPBY,TRUE);

		lEnableMenuItem(hProgramMenu,IDM_VIEW_AUTOARRANGE,TRUE);
		lCheckMenuItem(hProgramMenu,IDM_VIEW_AUTOARRANGE,tab.GetShellBrowser()->GetAutoArrange());
	}

	SetSortMenuItemStates(tab);
}