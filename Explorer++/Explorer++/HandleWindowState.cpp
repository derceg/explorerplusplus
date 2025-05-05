// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "App.h"
#include "Config.h"
#include "FeatureList.h"
#include "MainResource.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowser/ViewModes.h"
#include "ShellTreeView/ShellTreeView.h"
#include "SortMenuBuilder.h"
#include "TabContainerImpl.h"
#include "../Helper/MenuHelper.h"

void Explorerplusplus::UpdateWindowStates(const Tab &tab)
{
	UpdateDisplayWindow(tab);
}

/*
 * Set the state of the items in the main
 * program menu.
 */
void Explorerplusplus::SetProgramMenuItemStates(HMENU hProgramMenu)
{
	const Tab &tab = GetActivePane()->GetTabContainerImpl()->GetSelectedTab();

	ViewMode viewMode = tab.GetShellBrowserImpl()->GetViewMode();
	bool virtualFolder = tab.GetShellBrowserImpl()->InVirtualFolder();

	int numSelected = tab.GetShellBrowserImpl()->GetNumSelected();
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
	MenuHelper::EnableItem(hProgramMenu, IDM_FILE_DELETE,
		m_commandController.IsCommandEnabled(IDM_FILE_DELETE));
	MenuHelper::EnableItem(hProgramMenu, IDM_FILE_DELETEPERMANENTLY,
		m_commandController.IsCommandEnabled(IDM_FILE_DELETEPERMANENTLY));
	MenuHelper::EnableItem(hProgramMenu, IDM_FILE_PROPERTIES,
		m_commandController.IsCommandEnabled(IDM_FILE_PROPERTIES));

	MenuHelper::EnableItem(hProgramMenu, IDM_EDIT_UNDO, m_FileActionHandler.CanUndo());
	MenuHelper::EnableItem(hProgramMenu, IDM_EDIT_PASTE, CanPaste(PasteType::Normal));
	MenuHelper::EnableItem(hProgramMenu, IDM_EDIT_PASTESHORTCUT, CanPaste(PasteType::Shortcut));
	MenuHelper::EnableItem(hProgramMenu, IDM_EDIT_PASTEHARDLINK, CanPasteLink());
	MenuHelper::EnableItem(hProgramMenu, IDM_EDIT_PASTE_SYMBOLIC_LINK, CanPasteLink());

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

	if (m_app->GetFeatureList()->IsEnabled(Feature::DualPane))
	{
		MenuHelper::CheckItem(hProgramMenu, IDM_VIEW_DUAL_PANE, m_config->dualPane);
	}

	MenuHelper::CheckItem(hProgramMenu, IDM_VIEW_STATUSBAR, m_config->showStatusBar);
	MenuHelper::CheckItem(hProgramMenu, IDM_VIEW_FOLDERS, m_config->showFolders.get());
	MenuHelper::CheckItem(hProgramMenu, IDM_VIEW_DISPLAYWINDOW, m_config->showDisplayWindow.get());
	MenuHelper::CheckItem(hProgramMenu, IDM_VIEW_TOOLBARS_ADDRESS_BAR,
		m_config->showAddressBar.get());
	MenuHelper::CheckItem(hProgramMenu, IDM_VIEW_TOOLBARS_MAIN_TOOLBAR,
		m_config->showMainToolbar.get());
	MenuHelper::CheckItem(hProgramMenu, IDM_VIEW_TOOLBARS_BOOKMARKS_TOOLBAR,
		m_config->showBookmarksToolbar.get());
	MenuHelper::CheckItem(hProgramMenu, IDM_VIEW_TOOLBARS_DRIVES_TOOLBAR,
		m_config->showDrivesToolbar.get());
	MenuHelper::CheckItem(hProgramMenu, IDM_VIEW_TOOLBARS_APPLICATION_TOOLBAR,
		m_config->showApplicationToolbar.get());
	MenuHelper::CheckItem(hProgramMenu, IDM_VIEW_TOOLBARS_LOCK_TOOLBARS,
		m_config->lockToolbars.get());

	auto &mainFont = m_config->mainFont.get();
	MenuHelper::EnableItem(hProgramMenu, IDM_VIEW_DECREASE_TEXT_SIZE,
		!mainFont || mainFont->GetSize() > CustomFont::MINIMUM_SIZE);
	MenuHelper::EnableItem(hProgramMenu, IDM_VIEW_INCREASE_TEXT_SIZE,
		!mainFont || mainFont->GetSize() < CustomFont::MAXIMUM_SIZE);

	MenuHelper::CheckItem(hProgramMenu, IDM_VIEW_SHOWHIDDENFILES,
		tab.GetShellBrowserImpl()->GetShowHidden());
	MenuHelper::CheckItem(hProgramMenu, IDM_FILTER_APPLYFILTER,
		tab.GetShellBrowserImpl()->IsFilterApplied());

	MenuHelper::EnableItem(hProgramMenu, IDM_ACTIONS_NEWFOLDER, CanCreate());
	MenuHelper::EnableItem(hProgramMenu, IDM_ACTIONS_SPLITFILE,
		(tab.GetShellBrowserImpl()->GetNumSelectedFiles() == 1) && !virtualFolder);
	MenuHelper::EnableItem(hProgramMenu, IDM_ACTIONS_MERGEFILES,
		tab.GetShellBrowserImpl()->GetNumSelectedFiles() > 1);
	MenuHelper::EnableItem(hProgramMenu, IDM_ACTIONS_DESTROYFILES, anySelected);

	UINT itemToCheck = GetViewModeMenuId(viewMode);
	CheckMenuRadioItem(hProgramMenu, IDM_VIEW_EXTRALARGEICONS, IDM_VIEW_TILES, itemToCheck,
		MF_BYCOMMAND);

	MenuHelper::EnableItem(hProgramMenu, IDM_GO_BACK,
		m_commandController.IsCommandEnabled(IDM_GO_BACK));
	MenuHelper::EnableItem(hProgramMenu, IDM_GO_FORWARD,
		m_commandController.IsCommandEnabled(IDM_GO_FORWARD));
	MenuHelper::EnableItem(hProgramMenu, IDM_GO_UP,
		m_commandController.IsCommandEnabled(IDM_GO_UP));

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
			tab.GetShellBrowserImpl()->GetAutoArrange());
	}

	SortMenuBuilder sortMenuBuilder(m_app->GetResourceInstance());
	auto [sortByMenu, groupByMenu] = sortMenuBuilder.BuildMenus(tab);

	MenuHelper::AttachSubMenu(hProgramMenu, std::move(sortByMenu), IDM_VIEW_SORTBY, FALSE);
	MenuHelper::AttachSubMenu(hProgramMenu, std::move(groupByMenu), IDM_VIEW_GROUPBY, FALSE);
}
