// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainMenu.h"
#include "AppServices.h"
#include "Bookmarks/UI/BookmarksMainMenu.h"
#include "FeatureList.h"
#include "FrequentLocationsMenu.h"
#include "HistoryMenu.h"
#include "MainResource.h"
#include "MenuRanges.h"
#include "MenuView.h"
#include "ResourceIconModel.h"
#include "ResourceLoader.h"
#include "ShellBrowser/ViewModes.h"
#include "ShellIconModel.h"
#include "StockIconModel.h"
#include "TabRestorerMenu.h"
#include "ViewModeHelper.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/ShellHelper.h"

MainMenu::MainMenu(MenuView *menuView, BrowserWindow *browser, AppServices *appServices,
	ShellIconLoader *shellIconLoader, IconFetcher *iconFetcher) :
	MenuBase(menuView, appServices->GetAcceleratorManager()),
	m_resourceLoader(appServices->GetResourceLoader())
{
	// Some items on the main menu support drag and drop (e.g. bookmark items), so drag and drop is
	// enabled here. This is necessary, since drag and drop won't work if the style is simply set on
	// a submenu. For items that don't support drag and drop, setting this will have no effect.
	m_rootMenuView->EnableDragAndDrop(true);

	BuildMenu(browser, appServices->GetFeatureList(), appServices, shellIconLoader, iconFetcher);
}

void MainMenu::BuildMenu(BrowserWindow *browser, const FeatureList *featureList,
	AppServices *appServices, ShellIconLoader *shellIconLoader, IconFetcher *iconFetcher)
{
	BuildFileMenu(featureList, appServices, shellIconLoader);
	BuildEditMenu();
	BuildSelectionMenu();
	BuildViewMenu(featureList);
	BuildActionsMenu();
	BuildGoMenu(browser, appServices, shellIconLoader);
	BuildBookmarksMenu(browser, appServices, iconFetcher);
	BuildToolsMenu(featureList);
	BuildWindowMenu();
	BuildHelpMenu();
}

void MainMenu::BuildFileMenu(const FeatureList *featureList, AppServices *appServices,
	ShellIconLoader *shellIconLoader)
{
	auto *menuView = AppendSubMenu(m_rootMenuView, IDM_FILE_POPUP, IDS_FILE_POPUP);

	AppendItem(menuView, IDM_FILE_NEWTAB, IDS_FILE_NEWTAB);
	AppendItem(menuView, IDM_FILE_CLOSETAB, IDS_FILE_CLOSETAB);

	if (featureList->IsEnabled(Feature::MultipleWindowsPerSession))
	{
		AppendItem(menuView, IDM_FILE_NEW_WINDOW, IDS_FILE_NEW_WINDOW);
	}

	auto *recentTabsSubMenuView =
		AppendSubMenu(menuView, IDM_FILE_REOPEN_RECENT_TAB_POPUP, IDS_FILE_REOPEN_RECENT_TAB_POPUP);
	m_childMenus.push_back(std::make_unique<TabRestorerMenu>(recentTabsSubMenuView,
		m_acceleratorManager, appServices->GetTabRestorer(), shellIconLoader, m_resourceLoader,
		MENU_RECENT_TABS_START_ID, MENU_RECENT_TABS_END_ID));

	if (!featureList->IsEnabled(Feature::MultipleWindowsPerSession))
	{
		// TODO: Selecting clone window launches a separate process. That doesn't fit in with how
		// MultipleWindowsPerSession is designed to work and the menu item should be removed when
		// the feature is enabled by default.
		AppendItem(menuView, IDM_FILE_CLONEWINDOW, IDS_FILE_CLONEWINDOW);
	}

	menuView->AppendSeparator();

	AppendItem(menuView, IDM_FILE_SAVEDIRECTORYLISTING, IDS_FILE_SAVEDIRECTORYLISTING);
	AppendItem(menuView, IDM_FILE_OPENCOMMANDPROMPT, IDS_FILE_OPENCOMMANDPROMPT);
	AppendItem(menuView, IDM_FILE_OPENCOMMANDPROMPTADMINISTRATOR,
		IDS_FILE_OPENCOMMANDPROMPTADMINISTRATOR);
	AppendItem(menuView, IDM_FILE_COPYFOLDERPATH, IDS_FILE_COPYFOLDERPATH);
	AppendItem(menuView, IDM_FILE_COPYITEMPATH, IDS_FILE_COPYITEMPATH);
	AppendItem(menuView, IDM_FILE_COPYUNIVERSALFILEPATHS, IDS_FILE_COPYUNIVERSALFILEPATHS);
	AppendItem(menuView, IDM_FILE_COPYCOLUMNTEXT, IDS_FILE_COPYCOLUMNTEXT);

	menuView->AppendSeparator();

	AppendItem(menuView, IDM_FILE_SETFILEATTRIBUTES, IDS_FILE_SETFILEATTRIBUTES);
	AppendItem(menuView, IDM_FILE_DELETE, IDS_FILE_DELETE);
	AppendItem(menuView, IDM_FILE_DELETEPERMANENTLY, IDS_FILE_DELETEPERMANENTLY);
	AppendItem(menuView, IDM_FILE_RENAME, IDS_FILE_RENAME);
	AppendItem(menuView, IDM_FILE_PROPERTIES, IDS_FILE_PROPERTIES);

	menuView->AppendSeparator();

	AppendItem(menuView, IDM_FILE_EXIT, IDS_FILE_EXIT);
}

void MainMenu::BuildEditMenu()
{
	auto *menuView = AppendSubMenu(m_rootMenuView, IDM_EDIT_POPUP, IDS_EDIT_POPUP);

	AppendItem(menuView, IDM_EDIT_UNDO, IDS_EDIT_UNDO);

	menuView->AppendSeparator();

	AppendItem(menuView, IDM_EDIT_CUT, IDS_EDIT_CUT);
	AppendItem(menuView, IDM_EDIT_COPY, IDS_EDIT_COPY);
	AppendItem(menuView, IDM_EDIT_PASTE, IDS_EDIT_PASTE);
	AppendItem(menuView, IDM_EDIT_PASTESHORTCUT, IDS_EDIT_PASTESHORTCUT);
	AppendItem(menuView, IDM_EDIT_PASTEHARDLINK, IDS_EDIT_PASTEHARDLINK);
	AppendItem(menuView, IDM_EDIT_PASTE_SYMBOLIC_LINK, IDS_EDIT_PASTE_SYMBOLIC_LINK);

	menuView->AppendSeparator();

	AppendItem(menuView, IDM_EDIT_COPYTOFOLDER, IDS_EDIT_COPYTOFOLDER);
	AppendItem(menuView, IDM_EDIT_MOVETOFOLDER, IDS_EDIT_MOVETOFOLDER);

	menuView->AppendSeparator();

	AppendItem(menuView, IDM_EDIT_RESOLVELINK, IDS_EDIT_RESOLVELINK);
}

void MainMenu::BuildSelectionMenu()
{
	auto *menuView = AppendSubMenu(m_rootMenuView, IDM_SELECTION_POPUP, IDS_SELECTION_POPUP);

	AppendItem(menuView, IDM_EDIT_SELECTALL, IDS_EDIT_SELECTALL);
	AppendItem(menuView, IDM_EDIT_INVERTSELECTION, IDS_EDIT_INVERTSELECTION);
	AppendItem(menuView, IDM_EDIT_SELECTALLOFSAMETYPE, IDS_EDIT_SELECTALLOFSAMETYPE);
	AppendItem(menuView, IDM_EDIT_SELECTNONE, IDS_EDIT_SELECTNONE);
	AppendItem(menuView, IDM_EDIT_WILDCARDSELECTION, IDS_EDIT_WILDCARDSELECTION);
	AppendItem(menuView, IDM_EDIT_WILDCARDDESELECT, IDS_EDIT_WILDCARDDESELECT);
}

void MainMenu::BuildViewMenu(const FeatureList *featureList)
{
	auto *menuView = AppendSubMenu(m_rootMenuView, IDM_VIEW_POPUP, IDS_VIEW_POPUP);

	if (featureList->IsEnabled(Feature::DualPane))
	{
		AppendItem(menuView, IDM_VIEW_DUAL_PANE, IDS_VIEW_DUAL_PANE);
	}

	AppendItem(menuView, IDM_VIEW_STATUSBAR, IDS_VIEW_STATUSBAR);
	AppendItem(menuView, IDM_VIEW_FOLDERS, IDS_VIEW_FOLDERS);
	AppendItem(menuView, IDM_VIEW_DISPLAYWINDOW, IDS_VIEW_DISPLAYWINDOW);

	auto *toolbarsMenuView =
		AppendSubMenu(menuView, IDM_VIEW_TOOLBARS_POPUP, IDS_VIEW_TOOLBARS_POPUP);
	BuildToolbarsSubMenu(toolbarsMenuView);

	menuView->AppendSeparator();

	AppendItem(menuView, IDM_VIEW_DECREASE_TEXT_SIZE, IDS_VIEW_DECREASE_TEXT_SIZE);
	AppendItem(menuView, IDM_VIEW_INCREASE_TEXT_SIZE, IDS_VIEW_INCREASE_TEXT_SIZE);

	menuView->AppendSeparator();

	for (auto viewMode : VIEW_MODES)
	{
		AppendItem(menuView, GetViewModeMenuId(viewMode), GetViewModeMenuStringId(viewMode));
	}

	menuView->AppendSeparator();

	AppendItem(menuView, IDM_VIEW_AUTOARRANGE, IDS_VIEW_AUTOARRANGE);

	menuView->AppendSeparator();

	AppendItem(menuView, IDM_VIEW_SORTBY, IDS_VIEW_SORTBY);
	AppendItem(menuView, IDM_VIEW_GROUPBY, IDS_VIEW_GROUPBY);
	AppendItem(menuView, IDM_VIEW_SHOWHIDDENFILES, IDS_VIEW_SHOWHIDDENFILES);
	AppendItem(menuView, IDM_VIEW_REFRESH, IDS_VIEW_REFRESH);
	AppendItem(menuView, IDM_VIEW_SELECTCOLUMNS, IDS_VIEW_SELECTCOLUMNS);
	AppendItem(menuView, IDM_VIEW_AUTOSIZECOLUMNS, IDS_VIEW_AUTOSIZECOLUMNS);
	AppendItem(menuView, IDM_VIEW_SAVECOLUMNLAYOUTASDEFAULT, IDS_VIEW_SAVECOLUMNLAYOUTASDEFAULT);
	AppendItem(menuView, IDM_VIEW_CHANGEDISPLAYCOLOURS, IDS_VIEW_CHANGEDISPLAYCOLOURS);

	menuView->AppendSeparator();

	auto *filterMenuView = AppendSubMenu(menuView, IDM_VIEW_FILTER_POPUP, IDS_VIEW_FILTER_POPUP);
	BuildFilterSubMenu(filterMenuView);
}

void MainMenu::BuildToolbarsSubMenu(MenuView *toolbarsMenuView)
{
	AppendItem(toolbarsMenuView, IDM_VIEW_TOOLBARS_ADDRESS_BAR, IDS_VIEW_TOOLBARS_ADDRESS_BAR);
	AppendItem(toolbarsMenuView, IDM_VIEW_TOOLBARS_MAIN_TOOLBAR, IDS_VIEW_TOOLBARS_MAIN_TOOLBAR);
	AppendItem(toolbarsMenuView, IDM_VIEW_TOOLBARS_BOOKMARKS_TOOLBAR,
		IDS_VIEW_TOOLBARS_BOOKMARKS_TOOLBAR);
	AppendItem(toolbarsMenuView, IDM_VIEW_TOOLBARS_DRIVES_TOOLBAR,
		IDS_VIEW_TOOLBARS_DRIVES_TOOLBAR);
	AppendItem(toolbarsMenuView, IDM_VIEW_TOOLBARS_APPLICATION_TOOLBAR,
		IDS_VIEW_TOOLBARS_APPLICATION_TOOLBAR);

	toolbarsMenuView->AppendSeparator();

	AppendItem(toolbarsMenuView, IDM_VIEW_TOOLBARS_LOCK_TOOLBARS, IDS_VIEW_TOOLBARS_LOCK_TOOLBARS);
	AppendItem(toolbarsMenuView, IDM_VIEW_TOOLBARS_CUSTOMIZE, IDS_VIEW_TOOLBARS_CUSTOMIZE);
}

void MainMenu::BuildFilterSubMenu(MenuView *filterMenuView)
{
	AppendItem(filterMenuView, IDM_FILTER_FILTERRESULTS, IDS_FILTER_FILTERRESULTS);
	AppendItem(filterMenuView, IDM_FILTER_ENABLE_FILTER, IDS_FILTER_ENABLE_FILTER);
}

void MainMenu::BuildActionsMenu()
{
	auto *menuView = AppendSubMenu(m_rootMenuView, IDM_ACTIONS_POPUP, IDS_ACTIONS_POPUP);

	AppendItem(menuView, IDM_ACTIONS_NEWFOLDER, IDS_ACTIONS_NEWFOLDER);

	menuView->AppendSeparator();

	AppendItem(menuView, IDM_ACTIONS_SPLITFILE, IDS_ACTIONS_SPLITFILE);
	AppendItem(menuView, IDM_ACTIONS_MERGEFILES, IDS_ACTIONS_MERGEFILES);
	AppendItem(menuView, IDM_ACTIONS_DESTROYFILES, IDS_ACTIONS_DESTROYFILES);
}

void MainMenu::BuildGoMenu(BrowserWindow *browser, AppServices *appServices,
	ShellIconLoader *shellIconLoader)
{
	auto *menuView = AppendSubMenu(m_rootMenuView, IDM_GO_POPUP, IDS_GO_POPUP);

	AppendItem(menuView, IDM_GO_BACK, IDS_GO_BACK);
	AppendItem(menuView, IDM_GO_FORWARD, IDS_GO_FORWARD);
	AppendItem(menuView, IDM_GO_UP, IDS_GO_UP);

	menuView->AppendSeparator();

	auto *historySubMenuView = AppendSubMenu(menuView, IDM_GO_HISTORY_POPUP, IDS_GO_HISTORY_POPUP);
	m_childMenus.push_back(std::make_unique<HistoryMenu>(historySubMenuView, m_acceleratorManager,
		appServices->GetHistoryModel(), browser, shellIconLoader, MENU_HISTORY_START_ID,
		MENU_HISTORY_END_ID));

	auto *frequentLocationsSubMenuView =
		AppendSubMenu(menuView, IDM_GO_FREQUENT_LOCATIONS_POPUP, IDS_GO_FREQUENT_LOCATIONS_POPUP);
	m_childMenus.push_back(std::make_unique<FrequentLocationsMenu>(frequentLocationsSubMenuView,
		m_acceleratorManager, appServices->GetFrequentLocationsModel(), browser, shellIconLoader,
		MENU_FREQUENT_LOCATIONS_START_ID, MENU_FREQUENT_LOCATIONS_END_ID));

	menuView->AppendSeparator();

	// This is the quick access/home folder in Windows 10/11.
	AppendGoMenuItem(menuView, IDM_GO_QUICK_ACCESS, QUICK_ACCESS_PATH, shellIconLoader);
	AppendGoMenuItem(menuView, IDM_GO_COMPUTER, FOLDERID_ComputerFolder, shellIconLoader);

	menuView->AppendSeparator();

	AppendGoMenuItem(menuView, IDM_GO_DOCUMENTS, FOLDERID_Documents, shellIconLoader);
	AppendGoMenuItem(menuView, IDM_GO_DOWNLOADS, FOLDERID_Downloads, shellIconLoader);
	AppendGoMenuItem(menuView, IDM_GO_MUSIC, FOLDERID_Music, shellIconLoader);
	AppendGoMenuItem(menuView, IDM_GO_PICTURES, FOLDERID_Pictures, shellIconLoader);
	AppendGoMenuItem(menuView, IDM_GO_VIDEOS, FOLDERID_Videos, shellIconLoader);
	AppendGoMenuItem(menuView, IDM_GO_DESKTOP, FOLDERID_Desktop, shellIconLoader);

	menuView->AppendSeparator();

	AppendGoMenuItem(menuView, IDM_GO_RECYCLE_BIN, FOLDERID_RecycleBinFolder, shellIconLoader);
	AppendGoMenuItem(menuView, IDM_GO_CONTROL_PANEL, FOLDERID_ControlPanelFolder, shellIconLoader);
	AppendGoMenuItem(menuView, IDM_GO_PRINTERS, FOLDERID_PrintersFolder, shellIconLoader);
	AppendGoMenuItem(menuView, IDM_GO_NETWORK, FOLDERID_NetworkFolder, shellIconLoader);

	menuView->AppendSeparator();

	AppendGoMenuItem(menuView, IDM_GO_WSL_DISTRIBUTIONS, WSL_DISTRIBUTIONS_PATH, shellIconLoader);

	menuView->RemoveDuplicateSeparators();
	menuView->RemoveTrailingSeparators();
}

void MainMenu::AppendGoMenuItem(MenuView *goMenuView, UINT id, const KNOWNFOLDERID &folderId,
	ShellIconLoader *shellIconLoader)
{
	PidlAbsolute pidl;
	HRESULT hr = SHGetKnownFolderIDList(folderId, KF_FLAG_DEFAULT, nullptr, PidlOutParam(pidl));

	if (FAILED(hr))
	{
		return;
	}

	AppendGoMenuItem(goMenuView, id, pidl, shellIconLoader);
}

void MainMenu::AppendGoMenuItem(MenuView *goMenuView, UINT id, const std::wstring &path,
	ShellIconLoader *shellIconLoader)
{
	PidlAbsolute pidl;
	HRESULT hr = SHParseDisplayName(path.c_str(), nullptr, PidlOutParam(pidl), 0, nullptr);

	if (FAILED(hr))
	{
		return;
	}

	AppendGoMenuItem(goMenuView, id, pidl, shellIconLoader);
}

void MainMenu::AppendGoMenuItem(MenuView *goMenuView, UINT id, const PidlAbsolute &pidl,
	ShellIconLoader *shellIconLoader)
{
	std::wstring folderName;
	HRESULT hr = GetDisplayName(pidl.Raw(), SHGDN_INFOLDER, folderName);

	if (FAILED(hr))
	{
		return;
	}

	std::wstring folderPath;
	hr = GetDisplayName(pidl.Raw(), SHGDN_FORPARSING, folderPath);

	if (FAILED(hr))
	{
		return;
	}

	goMenuView->AppendItem(nullptr, id, folderName,
		std::make_unique<ShellIconModel>(shellIconLoader, pidl.Raw()), folderPath);
}

void MainMenu::BuildBookmarksMenu(BrowserWindow *browser, AppServices *appServices,
	IconFetcher *iconFetcher)
{
	auto *menuView = AppendSubMenu(m_rootMenuView, IDM_BOOKMARKS_POPUP, IDS_BOOKMARKS_POPUP);

	m_childMenus.push_back(std::make_unique<BookmarksMainMenu>(menuView, m_acceleratorManager,
		appServices->GetBookmarkTree(), browser, iconFetcher, appServices->GetPlatformContext(),
		m_resourceLoader, MENU_BOOKMARK_START_ID, MENU_BOOKMARK_END_ID));
}

void MainMenu::BuildToolsMenu(const FeatureList *featureList)
{
	auto *menuView = AppendSubMenu(m_rootMenuView, IDM_TOOLS_POPUP, IDS_TOOLS_POPUP);

	AppendItem(menuView, IDM_TOOLS_SEARCH, IDS_TOOLS_SEARCH);
	AppendItem(menuView, IDM_TOOLS_CUSTOMIZECOLORS, IDS_TOOLS_CUSTOMIZECOLORS);

	menuView->AppendSeparator();

	if (featureList->IsEnabled(Feature::Plugins))
	{
		AppendItem(menuView, IDM_TOOLS_RUNSCRIPT, IDS_TOOLS_RUNSCRIPT);
	}

	AppendItem(menuView, IDM_TOOLS_OPTIONS, IDS_TOOLS_OPTIONS);
}

void MainMenu::BuildWindowMenu()
{
	auto *menuView = AppendSubMenu(m_rootMenuView, IDM_WINDOW_POPUP, IDS_WINDOW_POPUP);

	AppendItem(menuView, IDM_WINDOW_SEARCH_TABS, IDS_WINDOW_SEARCH_TABS);
}

void MainMenu::BuildHelpMenu()
{
	auto *menuView = AppendSubMenu(m_rootMenuView, IDM_HELP_POPUP, IDS_HELP_POPUP);

	AppendItem(menuView, IDM_HELP_ONLINE_DOCUMENTATION, IDS_HELP_ONLINE_DOCUMENTATION);

	menuView->AppendSeparator();

	AppendItem(menuView, IDM_HELP_CHECKFORUPDATES, IDS_HELP_CHECKFORUPDATES);
	AppendItem(menuView, IDM_HELP_ABOUT, IDS_HELP_ABOUT);
}

void MainMenu::AppendItem(MenuView *menuView, UINT id, UINT textResourceId)
{
	auto helpText = m_resourceLoader->MaybeLoadString(id);
	menuView->AppendItem(nullptr, id, m_resourceLoader->LoadString(textResourceId),
		MaybeCreateIconModel(id), helpText.value_or(L""), GetAcceleratorTextForId(id));
}

MenuView *MainMenu::AppendSubMenu(MenuView *menuView, UINT id, UINT textResourceId)
{
	return menuView->AppendSubMenu(nullptr, id, m_resourceLoader->LoadString(textResourceId),
		MaybeCreateIconModel(id));
}

std::unique_ptr<IconModel> MainMenu::MaybeCreateIconModel(UINT id) const
{
	// Creating a symlink typically requires elevation. However, if the application is already
	// elevated, there's no need to show the shield icon (which is used to indicate that elevation
	// is required).
	//
	// Note that elevation isn't required if developer mode is enabled on Windows 10 and above.
	// Since the status of developer mode isn't checked here, the shield icon may be shown in cases
	// where elevation isn't actually required. That's not too much of an issue, since the icon here
	// is considered to be a hint that elevation may be required.
	if (id == IDM_EDIT_PASTE_SYMBOLIC_LINK && !IsProcessElevated())
	{
		return std::make_unique<StockIconModel>(SIID_SHIELD, IconSize::Small);
	}

	auto icon = GetIconForCommand(id);

	if (!icon)
	{
		return nullptr;
	}

	return std::make_unique<ResourceIconModel>(*icon, IconSize::Small, m_resourceLoader);
}

std::optional<Icon> MainMenu::GetIconForCommand(UINT id)
{
	switch (id)
	{
	case IDM_FILE_NEWTAB:
		return Icon::NewTab;

	case IDM_FILE_CLOSETAB:
		return Icon::CloseTab;

	case IDM_FILE_OPENCOMMANDPROMPT:
		return Icon::CommandLine;

	case IDM_FILE_OPENCOMMANDPROMPTADMINISTRATOR:
		return Icon::CommandLineAdmin;

	case IDM_FILE_DELETE:
		return Icon::Delete;

	case IDM_FILE_DELETEPERMANENTLY:
		return Icon::DeletePermanently;

	case IDM_FILE_RENAME:
		return Icon::Rename;

	case IDM_FILE_PROPERTIES:
		return Icon::Properties;

	case IDM_EDIT_UNDO:
		return Icon::Undo;

	case IDM_EDIT_COPY:
		return Icon::Copy;

	case IDM_EDIT_CUT:
		return Icon::Cut;

	case IDM_EDIT_PASTE:
		return Icon::Paste;

	case IDM_EDIT_PASTESHORTCUT:
		return Icon::PasteShortcut;

	case IDM_EDIT_COPYTOFOLDER:
		return Icon::CopyTo;

	case IDM_EDIT_MOVETOFOLDER:
		return Icon::MoveTo;

	case IDM_ACTIONS_NEWFOLDER:
		return Icon::NewFolder;

	case IDM_ACTIONS_SPLITFILE:
		return Icon::SplitFiles;

	case IDM_ACTIONS_MERGEFILES:
		return Icon::MergeFiles;

	case IDM_VIEW_REFRESH:
		return Icon::Refresh;

	case IDM_VIEW_SELECTCOLUMNS:
		return Icon::SelectColumns;

	case IDM_FILTER_FILTERRESULTS:
		return Icon::Filter;

	case IDM_GO_BACK:
		return Icon::Back;

	case IDM_GO_FORWARD:
		return Icon::Forward;

	case IDM_GO_UP:
		return Icon::Up;

	case IDM_TOOLS_SEARCH:
		return Icon::Search;

	case IDM_TOOLS_CUSTOMIZECOLORS:
		return Icon::CustomizeColors;

	case IDM_TOOLS_OPTIONS:
		return Icon::Options;

	case IDM_HELP_ONLINE_DOCUMENTATION:
		return Icon::Help;

	default:
		return std::nullopt;
	}
}
