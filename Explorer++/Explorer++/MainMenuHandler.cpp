// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AppServices.h"
#include "Config.h"
#include "DestroyFilesDialog.h"
#include "FeatureList.h"
#include "MainMenu.h"
#include "MainMenuHost.h"
#include "MainResource.h"
#include "ModelessDialogHelper.h"
#include "OptionsDialog.h"
#include "SelectColumnsDialog.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "SortMenuBuilder.h"
#include "TabContainer.h"
#include "../Helper/Helper.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/PidlHelper.h"
#include "../Helper/ShellHelper.h"
#include <wil/com.h>
#include <Shlwapi.h>

void Explorerplusplus::InitializeMainMenu()
{
	m_mainMenuHost = std::make_unique<MainMenuHost>(this);
	m_mainMenu = std::make_unique<MainMenu>(m_mainMenuHost->GetView(), this, m_appServices,
		&m_shellIconLoader, &m_iconFetcher);

	auto res = SetMenu(m_hwnd, m_mainMenuHost->ReleaseMenu());
	CHECK(res);
}

void Explorerplusplus::OnInitMenu(HMENU menu)
{
	// Note that as per
	// https://stackoverflow.com/questions/69917594/wm-initmenu-has-unexpected-wparam-for-system-menu#comment123596433_69917594,
	// the menu parameter passed to WM_INITMENU will be the main menu, even if the user is selecting
	// an item from the system menu. Additionally, WM_INITMENU will be sent simply when clicking a
	// blank spot the menu bar. That can result in some unnecessary work (to update the state of the
	// main menu), but shouldn't have any functional issues.
	if (menu == GetMenu(m_hwnd))
	{
		SetMainMenuItemStates(menu);
	}
}

void Explorerplusplus::SetMainMenuItemStates(HMENU mainMenu)
{
	const Tab &tab = GetActivePane()->GetTabContainer()->GetSelectedTab();

	ViewMode viewMode = tab.GetShellBrowser()->GetViewMode();

	int numSelected = tab.GetShellBrowserImpl()->GetNumSelected();
	bool anySelected = (numSelected > 0);

	MenuHelper::EnableItem(mainMenu, IDM_FILE_COPYITEMPATH,
		m_commandController.IsCommandEnabled(IDM_FILE_COPYITEMPATH));
	MenuHelper::EnableItem(mainMenu, IDM_FILE_COPYUNIVERSALFILEPATHS,
		m_commandController.IsCommandEnabled(IDM_FILE_COPYUNIVERSALFILEPATHS));
	MenuHelper::EnableItem(mainMenu, IDM_FILE_SETFILEATTRIBUTES,
		m_commandController.IsCommandEnabled(IDM_FILE_SETFILEATTRIBUTES));
	MenuHelper::EnableItem(mainMenu, IDM_FILE_OPENCOMMANDPROMPT,
		m_commandController.IsCommandEnabled(IDM_FILE_OPENCOMMANDPROMPT));
	MenuHelper::EnableItem(mainMenu, IDM_FILE_OPENCOMMANDPROMPTADMINISTRATOR,
		m_commandController.IsCommandEnabled(IDM_FILE_OPENCOMMANDPROMPTADMINISTRATOR));
	MenuHelper::EnableItem(mainMenu, IDM_FILE_SAVEDIRECTORYLISTING,
		m_commandController.IsCommandEnabled(IDM_FILE_SAVEDIRECTORYLISTING));
	MenuHelper::EnableItem(mainMenu, IDM_FILE_COPYCOLUMNTEXT,
		anySelected && (viewMode == +ViewMode::Details));

	MenuHelper::EnableItem(mainMenu, IDM_FILE_RENAME,
		m_commandController.IsCommandEnabled(IDM_FILE_RENAME));
	MenuHelper::EnableItem(mainMenu, IDM_FILE_DELETE,
		m_commandController.IsCommandEnabled(IDM_FILE_DELETE));
	MenuHelper::EnableItem(mainMenu, IDM_FILE_DELETEPERMANENTLY,
		m_commandController.IsCommandEnabled(IDM_FILE_DELETEPERMANENTLY));
	MenuHelper::EnableItem(mainMenu, IDM_FILE_PROPERTIES,
		m_commandController.IsCommandEnabled(IDM_FILE_PROPERTIES));

	MenuHelper::EnableItem(mainMenu, IDM_EDIT_UNDO, m_fileActionHandler.CanUndo());
	MenuHelper::EnableItem(mainMenu, IDM_EDIT_PASTE, CanPaste(PasteType::Normal));
	MenuHelper::EnableItem(mainMenu, IDM_EDIT_PASTESHORTCUT, CanPaste(PasteType::Shortcut));
	MenuHelper::EnableItem(mainMenu, IDM_EDIT_PASTEHARDLINK, CanPasteLink());
	MenuHelper::EnableItem(mainMenu, IDM_EDIT_PASTE_SYMBOLIC_LINK, CanPasteLink());

	MenuHelper::EnableItem(mainMenu, IDM_EDIT_CUT,
		m_commandController.IsCommandEnabled(IDM_EDIT_CUT));
	MenuHelper::EnableItem(mainMenu, IDM_EDIT_COPY,
		m_commandController.IsCommandEnabled(IDM_EDIT_COPY));
	MenuHelper::EnableItem(mainMenu, IDM_EDIT_MOVETOFOLDER,
		m_commandController.IsCommandEnabled(IDM_EDIT_MOVETOFOLDER));
	MenuHelper::EnableItem(mainMenu, IDM_EDIT_COPYTOFOLDER,
		m_commandController.IsCommandEnabled(IDM_EDIT_COPYTOFOLDER));
	MenuHelper::EnableItem(mainMenu, IDM_EDIT_WILDCARDDESELECT,
		m_commandController.IsCommandEnabled(IDM_EDIT_WILDCARDDESELECT));
	MenuHelper::EnableItem(mainMenu, IDM_EDIT_SELECTNONE,
		m_commandController.IsCommandEnabled(IDM_EDIT_SELECTNONE));
	MenuHelper::EnableItem(mainMenu, IDM_EDIT_RESOLVELINK, anySelected);

	if (m_featureList->IsEnabled(Feature::DualPane))
	{
		MenuHelper::CheckItem(mainMenu, IDM_VIEW_DUAL_PANE, m_config->dualPane);
	}

	MenuHelper::CheckItem(mainMenu, IDM_VIEW_STATUSBAR, m_config->showStatusBar.get());
	MenuHelper::CheckItem(mainMenu, IDM_VIEW_FOLDERS, m_config->showFolders.get());
	MenuHelper::CheckItem(mainMenu, IDM_VIEW_DISPLAYWINDOW, m_config->showDisplayWindow.get());
	MenuHelper::CheckItem(mainMenu, IDM_VIEW_TOOLBARS_ADDRESS_BAR, m_config->showAddressBar.get());
	MenuHelper::CheckItem(mainMenu, IDM_VIEW_TOOLBARS_MAIN_TOOLBAR,
		m_config->showMainToolbar.get());
	MenuHelper::CheckItem(mainMenu, IDM_VIEW_TOOLBARS_BOOKMARKS_TOOLBAR,
		m_config->showBookmarksToolbar.get());
	MenuHelper::CheckItem(mainMenu, IDM_VIEW_TOOLBARS_DRIVES_TOOLBAR,
		m_config->showDrivesToolbar.get());
	MenuHelper::CheckItem(mainMenu, IDM_VIEW_TOOLBARS_APPLICATION_TOOLBAR,
		m_config->showApplicationToolbar.get());
	MenuHelper::CheckItem(mainMenu, IDM_VIEW_TOOLBARS_LOCK_TOOLBARS, m_config->lockToolbars.get());

	MenuHelper::EnableItem(mainMenu, IDM_VIEW_DECREASE_TEXT_SIZE,
		m_commandController.IsCommandEnabled(IDM_VIEW_DECREASE_TEXT_SIZE));
	MenuHelper::EnableItem(mainMenu, IDM_VIEW_INCREASE_TEXT_SIZE,
		m_commandController.IsCommandEnabled(IDM_VIEW_INCREASE_TEXT_SIZE));

	MenuHelper::CheckItem(mainMenu, IDM_VIEW_SHOWHIDDENFILES,
		tab.GetShellBrowserImpl()->GetShowHidden());
	MenuHelper::CheckItem(mainMenu, IDM_FILTER_ENABLE_FILTER,
		tab.GetShellBrowserImpl()->IsFilterEnabled());

	MenuHelper::EnableItem(mainMenu, IDM_ACTIONS_NEWFOLDER,
		m_commandController.IsCommandEnabled(IDM_ACTIONS_NEWFOLDER));
	MenuHelper::EnableItem(mainMenu, IDM_ACTIONS_SPLITFILE,
		m_commandController.IsCommandEnabled(IDM_ACTIONS_SPLITFILE));
	MenuHelper::EnableItem(mainMenu, IDM_ACTIONS_MERGEFILES,
		m_commandController.IsCommandEnabled(IDM_ACTIONS_MERGEFILES));
	MenuHelper::EnableItem(mainMenu, IDM_ACTIONS_DESTROYFILES, anySelected);

	UINT itemToCheck = GetViewModeMenuId(viewMode);
	CheckMenuRadioItem(mainMenu, IDM_VIEW_EXTRALARGEICONS, IDM_VIEW_TILES, itemToCheck,
		MF_BYCOMMAND);

	MenuHelper::EnableItem(mainMenu, IDM_GO_BACK,
		m_commandController.IsCommandEnabled(IDM_GO_BACK));
	MenuHelper::EnableItem(mainMenu, IDM_GO_FORWARD,
		m_commandController.IsCommandEnabled(IDM_GO_FORWARD));
	MenuHelper::EnableItem(mainMenu, IDM_GO_UP, m_commandController.IsCommandEnabled(IDM_GO_UP));

	MenuHelper::EnableItem(mainMenu, IDM_VIEW_AUTOSIZECOLUMNS,
		m_commandController.IsCommandEnabled(IDM_VIEW_AUTOSIZECOLUMNS));

	if (viewMode == +ViewMode::Details)
	{
		MenuHelper::EnableItem(mainMenu, IDM_VIEW_AUTOARRANGE, FALSE);
		MenuHelper::CheckItem(mainMenu, IDM_VIEW_AUTOARRANGE, FALSE);

		MenuHelper::EnableItem(mainMenu, IDM_VIEW_GROUPBY, TRUE);
	}
	else if (viewMode == +ViewMode::List)
	{
		MenuHelper::EnableItem(mainMenu, IDM_VIEW_GROUPBY, FALSE);

		MenuHelper::EnableItem(mainMenu, IDM_VIEW_AUTOARRANGE, FALSE);
		MenuHelper::CheckItem(mainMenu, IDM_VIEW_AUTOARRANGE, FALSE);
	}
	else
	{
		MenuHelper::EnableItem(mainMenu, IDM_VIEW_GROUPBY, TRUE);

		MenuHelper::EnableItem(mainMenu, IDM_VIEW_AUTOARRANGE, TRUE);
		MenuHelper::CheckItem(mainMenu, IDM_VIEW_AUTOARRANGE,
			tab.GetShellBrowser()->IsAutoArrangeEnabled());
	}

	SortMenuBuilder sortMenuBuilder(m_resourceLoader);
	auto [sortByMenu, groupByMenu] = sortMenuBuilder.BuildMenus(tab);

	MenuHelper::AttachSubMenu(mainMenu, std::move(sortByMenu), IDM_VIEW_SORTBY, FALSE);
	MenuHelper::AttachSubMenu(mainMenu, std::move(groupByMenu), IDM_VIEW_GROUPBY, FALSE);
}

bool Explorerplusplus::MaybeHandleMainMenuItemSelection(UINT id)
{
	if (!m_mainMenuHost->CanHandleSelection(id))
	{
		return false;
	}

	m_mainMenuHost->SelectItem(id, IsKeyDown(VK_CONTROL), IsKeyDown(VK_SHIFT));

	return true;
}

void Explorerplusplus::OnDestroyFiles()
{
	std::list<std::wstring> fullFilenameList;
	int iItem = -1;

	while ((iItem = ListView_GetNextItem(m_hActiveListView, iItem, LVNI_SELECTED)) != -1)
	{
		std::wstring fullFilename = m_pActiveShellBrowser->GetItemFullName(iItem);
		fullFilenameList.push_back(fullFilename);
	}

	auto *destroyFilesDialog = DestroyFilesDialog::Create(m_resourceLoader, m_hwnd,
		fullFilenameList, m_config->globalFolderSettings.showFriendlyDates);
	destroyFilesDialog->ShowModalDialog();
}

void Explorerplusplus::OnShowOptions()
{
	CreateOrSwitchToModelessDialog(m_appServices->GetModelessDialogList(), L"OptionsDialog",
		[this]
		{
			return OptionsDialog::Create(m_resourceLoader, m_hwnd,
				m_appServices->GetAppController(), m_config, m_appServices->GetDarkModeManager(),
				m_appServices->GetThemeManager(), this);
		});
}

void Explorerplusplus::OnResolveLink()
{
	TCHAR szFullFileName[MAX_PATH];
	TCHAR szPath[MAX_PATH];
	HRESULT hr;
	int iItem;

	iItem = ListView_GetNextItem(m_hActiveListView, -1, LVNI_FOCUSED);

	if (iItem != -1)
	{
		std::wstring shortcutFileName = m_pActiveShellBrowser->GetItemFullName(iItem);

		hr = FileOperations::ResolveLink(m_hwnd, 0, shortcutFileName.c_str(), szFullFileName,
			std::size(szFullFileName));

		if (hr == S_OK)
		{
			/* Strip the filename, just leaving the path component. */
			StringCchCopy(szPath, std::size(szPath), szFullFileName);
			PathRemoveFileSpec(szPath);

			Tab &newTab =
				GetActivePane()->GetTabContainer()->CreateNewTab(szPath, { .selected = true });

			if (newTab.GetShellBrowser()->GetDirectoryPath() == szPath)
			{
				wil::com_ptr_nothrow<IShellFolder> parent;
				hr = SHBindToObject(nullptr, newTab.GetShellBrowser()->GetDirectory().Raw(),
					nullptr, IID_PPV_ARGS(&parent));

				if (hr == S_OK)
				{
					auto *filename = PathFindFileName(szFullFileName);
					assert(filename != szFullFileName);

					PidlAbsolute pidl;
					hr = CreateSimplePidl(filename, pidl, parent.get());

					if (SUCCEEDED(hr))
					{
						m_pActiveShellBrowser->SelectItems({ pidl.Raw() });
					}
				}
			}

			SetFocus(m_hActiveListView);
		}
	}
}

void Explorerplusplus::OnGoToOffset(int offset)
{
	Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	selectedTab.GetShellBrowserImpl()->GetNavigationController()->GoToOffset(offset);
}

void Explorerplusplus::OnSelectColumns()
{
	auto *selectColumnsDialog = SelectColumnsDialog::Create(m_resourceLoader, m_hwnd,
		GetActivePane()->GetTabContainer()->GetSelectedTab().GetShellBrowserImpl());
	selectColumnsDialog->ShowModalDialog();
}
