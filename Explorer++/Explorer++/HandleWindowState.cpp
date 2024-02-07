// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "FeatureList.h"
#include "MainResource.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowser/ViewModes.h"
#include "ShellTreeView/ShellTreeView.h"
#include "SortMenuBuilder.h"
#include "TabContainer.h"
#include "../Helper/MenuHelper.h"

void Explorerplusplus::UpdateWindowStates(const Tab &tab)
{
	UpdateStatusBarText(tab);
	UpdateDisplayWindow(tab);
}

/*
 * Set the state of the items in the main
 * program menu.
 */
void Explorerplusplus::SetProgramMenuItemStates(HMENU hProgramMenu)
{
	const Tab &tab = GetActivePane()->GetTabContainer()->GetSelectedTab();

	ViewMode viewMode = tab.GetShellBrowser()->GetViewMode();
	bool virtualFolder = tab.GetShellBrowser()->InVirtualFolder();

	int numSelected = tab.GetShellBrowser()->GetNumSelected();
	bool anySelected = (numSelected > 0);

	MenuHelper::EnableItem(hProgramMenu, IDM_FILE_COPYITEMPATH, AnyItemsSelected());
	MenuHelper::EnableItem(hProgramMenu, IDM_FILE_COPYUNIVERSALFILEPATHS, AnyItemsSelected());
	MenuHelper::EnableItem(hProgramMenu, IDM_FILE_SETFILEATTRIBUTES, AnyItemsSelected());
	MenuHelper::EnableItem(hProgramMenu, IDM_FILE_OPENCOMMANDPROMPT, !virtualFolder);
	MenuHelper::EnableItem(hProgramMenu, IDM_FILE_OPENCOMMANDPROMPTADMINISTRATOR, !virtualFolder);
	MenuHelper::EnableItem(hProgramMenu, IDM_FILE_SAVEDIRECTORYLISTING, !virtualFolder);
	MenuHelper::EnableItem(hProgramMenu, IDM_FILE_COPYCOLUMNTEXT,
		anySelected && (viewMode == +ViewMode::Details));

	MenuHelper::EnableItem(hProgramMenu, IDM_FILE_RENAME, CanRename());
	MenuHelper::EnableItem(hProgramMenu, IDM_FILE_DELETE, CanDelete());
	MenuHelper::EnableItem(hProgramMenu, IDM_FILE_DELETEPERMANENTLY, CanDelete());
	MenuHelper::EnableItem(hProgramMenu, IDM_FILE_PROPERTIES, CanShowFileProperties());

	MenuHelper::EnableItem(hProgramMenu, IDM_EDIT_UNDO, m_FileActionHandler.CanUndo());
	MenuHelper::EnableItem(hProgramMenu, IDM_EDIT_PASTE, CanPaste());
	MenuHelper::EnableItem(hProgramMenu, IDM_EDIT_PASTESHORTCUT, CanPasteShortcut());
	MenuHelper::EnableItem(hProgramMenu, IDM_EDIT_PASTEHARDLINK, CanPaste());

	/* The following menu items are only enabled when one
	or more files are selected (they represent file
	actions, cut/copy, etc). */
	MenuHelper::EnableItem(hProgramMenu, IDM_EDIT_COPY, CanCopy());
	MenuHelper::EnableItem(hProgramMenu, IDM_EDIT_CUT, CanCut());
	MenuHelper::EnableItem(hProgramMenu, IDM_EDIT_COPYTOFOLDER,
		CanCopy() && GetFocus() != m_shellTreeView->GetHWND());
	MenuHelper::EnableItem(hProgramMenu, IDM_EDIT_MOVETOFOLDER,
		CanCut() && GetFocus() != m_shellTreeView->GetHWND());
	MenuHelper::EnableItem(hProgramMenu, IDM_EDIT_WILDCARDDESELECT, anySelected);
	MenuHelper::EnableItem(hProgramMenu, IDM_EDIT_SELECTNONE, anySelected);
	MenuHelper::EnableItem(hProgramMenu, IDM_EDIT_RESOLVELINK, anySelected);

	if (FeatureList::GetInstance()->IsEnabled(Feature::DualPane))
	{
		MenuHelper::CheckItem(hProgramMenu, IDM_VIEW_DUAL_PANE, m_config->dualPane);
	}

	MenuHelper::CheckItem(hProgramMenu, IDM_VIEW_STATUSBAR, m_config->showStatusBar);
	MenuHelper::CheckItem(hProgramMenu, IDM_VIEW_FOLDERS, m_config->showFolders.get());
	MenuHelper::CheckItem(hProgramMenu, IDM_VIEW_DISPLAYWINDOW, m_config->showDisplayWindow);
	MenuHelper::CheckItem(hProgramMenu, IDM_TOOLBARS_ADDRESSBAR, m_config->showAddressBar);
	MenuHelper::CheckItem(hProgramMenu, IDM_TOOLBARS_MAINTOOLBAR, m_config->showMainToolbar);
	MenuHelper::CheckItem(hProgramMenu, IDM_TOOLBARS_BOOKMARKSTOOLBAR,
		m_config->showBookmarksToolbar);
	MenuHelper::CheckItem(hProgramMenu, IDM_TOOLBARS_DRIVES, m_config->showDrivesToolbar);
	MenuHelper::CheckItem(hProgramMenu, IDM_TOOLBARS_APPLICATIONTOOLBAR,
		m_config->showApplicationToolbar);
	MenuHelper::CheckItem(hProgramMenu, IDM_TOOLBARS_LOCKTOOLBARS, m_config->lockToolbars);

	auto &mainFont = m_config->mainFont.get();
	MenuHelper::EnableItem(hProgramMenu, IDM_VIEW_DECREASE_TEXT_SIZE,
		!mainFont || mainFont->GetSize() > CustomFont::MINIMUM_SIZE);
	MenuHelper::EnableItem(hProgramMenu, IDM_VIEW_INCREASE_TEXT_SIZE,
		!mainFont || mainFont->GetSize() < CustomFont::MAXIMUM_SIZE);

	MenuHelper::CheckItem(hProgramMenu, IDM_VIEW_SHOWHIDDENFILES,
		tab.GetShellBrowser()->GetShowHidden());
	MenuHelper::CheckItem(hProgramMenu, IDM_FILTER_APPLYFILTER,
		tab.GetShellBrowser()->IsFilterApplied());

	MenuHelper::EnableItem(hProgramMenu, IDM_ACTIONS_NEWFOLDER, CanCreate());
	MenuHelper::EnableItem(hProgramMenu, IDM_ACTIONS_SPLITFILE,
		(tab.GetShellBrowser()->GetNumSelectedFiles() == 1) && !virtualFolder);
	MenuHelper::EnableItem(hProgramMenu, IDM_ACTIONS_MERGEFILES,
		tab.GetShellBrowser()->GetNumSelectedFiles() > 1);
	MenuHelper::EnableItem(hProgramMenu, IDM_ACTIONS_DESTROYFILES, anySelected);

	UINT itemToCheck = GetViewModeMenuId(viewMode);
	CheckMenuRadioItem(hProgramMenu, IDM_VIEW_THUMBNAILS, IDM_VIEW_EXTRALARGEICONS, itemToCheck,
		MF_BYCOMMAND);

	MenuHelper::EnableItem(hProgramMenu, IDM_GO_BACK,
		tab.GetShellBrowser()->GetNavigationController()->CanGoBack());
	MenuHelper::EnableItem(hProgramMenu, IDM_GO_FORWARD,
		tab.GetShellBrowser()->GetNavigationController()->CanGoForward());
	MenuHelper::EnableItem(hProgramMenu, IDM_GO_UP,
		tab.GetShellBrowser()->GetNavigationController()->CanGoUp());

	MenuHelper::EnableItem(hProgramMenu, IDM_VIEW_AUTOSIZECOLUMNS, viewMode == +ViewMode::Details);

	if (viewMode == +ViewMode::Details)
	{
		/* Disable auto arrange menu item. */
		MenuHelper::EnableItem(hProgramMenu, IDM_VIEW_AUTOARRANGE, FALSE);
		MenuHelper::CheckItem(hProgramMenu, IDM_VIEW_AUTOARRANGE, FALSE);

		MenuHelper::EnableItem(hProgramMenu, IDM_VIEW_GROUPBY, TRUE);
	}
	else if (viewMode == +ViewMode::List)
	{
		/* Disable group menu item. */
		MenuHelper::EnableItem(hProgramMenu, IDM_VIEW_GROUPBY, FALSE);

		/* Disable auto arrange menu item. */
		MenuHelper::EnableItem(hProgramMenu, IDM_VIEW_AUTOARRANGE, FALSE);
		MenuHelper::CheckItem(hProgramMenu, IDM_VIEW_AUTOARRANGE, FALSE);
	}
	else
	{
		MenuHelper::EnableItem(hProgramMenu, IDM_VIEW_GROUPBY, TRUE);

		MenuHelper::EnableItem(hProgramMenu, IDM_VIEW_AUTOARRANGE, TRUE);
		MenuHelper::CheckItem(hProgramMenu, IDM_VIEW_AUTOARRANGE,
			tab.GetShellBrowser()->GetAutoArrange());
	}

	SortMenuBuilder sortMenuBuilder(m_resourceInstance);
	auto [sortByMenu, groupByMenu] = sortMenuBuilder.BuildMenus(tab);

	MenuHelper::AttachSubMenu(hProgramMenu, std::move(sortByMenu), IDM_VIEW_SORTBY, FALSE);
	MenuHelper::AttachSubMenu(hProgramMenu, std::move(groupByMenu), IDM_VIEW_GROUPBY, FALSE);
}
