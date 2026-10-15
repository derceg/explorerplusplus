// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainMenu.h"
#include "AppServices.h"
#include "Bookmarks/UI/BookmarksMainMenu.h"
#include "BrowserCommandController.h"
#include "BrowserWindow.h"
#include "Config.h"
#include "CoreInterface.h"
#include "FeatureList.h"
#include "FrequentLocationsMenu.h"
#include "HistoryMenu.h"
#include "MainResource.h"
#include "MenuView.h"
#include "PlatformContext.h"
#include "ResourceIconModel.h"
#include "ResourceLoader.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellIconModel.h"
#include "SortGroupMenu.h"
#include "StockIconModel.h"
#include "TabContainer.h"
#include "TabRestorerMenu.h"
#include "ToolbarOptionsMenu.h"
#include "ToolsMenu.h"
#include "ViewsMenu.h"
#include "../Helper/FileActionHandler.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/ShellHelper.h"

MainMenu::MainMenu(MenuView *menuView, BrowserWindow *browser, AppServices *appServices,
	ShellIconLoader *shellIconLoader, IconFetcher *iconFetcher,
	const FileActionHandler *fileActionHandler, CoreInterface *coreInterface) :
	MenuBase(menuView, appServices->GetAcceleratorManager()),
	m_browser(browser),
	m_config(appServices->GetConfig()),
	m_clipboardStore(appServices->GetPlatformContext()->GetClipboardStore()),
	m_fileActionHandler(fileActionHandler),
	m_resourceLoader(appServices->GetResourceLoader()),
	m_coreInterface(coreInterface)
{
	BuildMenu(browser, appServices, shellIconLoader, iconFetcher);
}

void MainMenu::BuildMenu(BrowserWindow *browser, AppServices *appServices,
	ShellIconLoader *shellIconLoader, IconFetcher *iconFetcher)
{
	BuildFileMenu(appServices->GetTabRestorer(), appServices->GetFeatureList(), shellIconLoader);
	BuildEditMenu();
	BuildSelectionMenu();
	BuildViewMenu(browser, appServices);
	BuildActionsMenu();
	BuildGoMenu(browser, appServices, shellIconLoader);
	BuildBookmarksMenu(browser, appServices, iconFetcher);
	BuildToolsMenu(browser, appServices->GetFeatureList(),
		appServices->GetPluginMenuItemsRegistry());
	BuildWindowMenu();
	BuildHelpMenu();
}

void MainMenu::BuildFileMenu(TabRestorer *tabRestorer, const FeatureList *featureList,
	ShellIconLoader *shellIconLoader)
{
	auto *menuView = AppendSubMenu(m_rootMenuView, IDS_FILE_POPUP);

	AppendItem(menuView, IDM_FILE_NEWTAB, IDS_FILE_NEWTAB);
	AppendItem(menuView, IDM_FILE_CLOSETAB, IDS_FILE_CLOSETAB);

	if (featureList->IsEnabled(Feature::MultipleWindowsPerSession))
	{
		AppendItem(menuView, IDM_FILE_NEW_WINDOW, IDS_FILE_NEW_WINDOW);
	}

	auto *recentTabsSubMenuView = AppendSubMenu(menuView, IDS_FILE_REOPEN_RECENT_TAB_POPUP);
	m_childMenus.push_back(std::make_unique<TabRestorerMenu>(recentTabsSubMenuView,
		m_acceleratorManager, tabRestorer, shellIconLoader, m_resourceLoader));

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
	auto *menuView = AppendSubMenu(m_rootMenuView, IDS_EDIT_POPUP);

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
	auto *menuView = AppendSubMenu(m_rootMenuView, IDS_SELECTION_POPUP);

	AppendItem(menuView, IDM_EDIT_SELECTALL, IDS_EDIT_SELECTALL);
	AppendItem(menuView, IDM_EDIT_INVERTSELECTION, IDS_EDIT_INVERTSELECTION);
	AppendItem(menuView, IDM_EDIT_SELECTALLOFSAMETYPE, IDS_EDIT_SELECTALLOFSAMETYPE);
	AppendItem(menuView, IDM_EDIT_SELECTNONE, IDS_EDIT_SELECTNONE);
	AppendItem(menuView, IDM_EDIT_WILDCARDSELECTION, IDS_EDIT_WILDCARDSELECTION);
	AppendItem(menuView, IDM_EDIT_WILDCARDDESELECT, IDS_EDIT_WILDCARDDESELECT);
}

void MainMenu::BuildViewMenu(BrowserWindow *browser, AppServices *appServices)
{
	auto *menuView = AppendSubMenu(m_rootMenuView, IDS_VIEW_POPUP);

	if (appServices->GetFeatureList()->IsEnabled(Feature::DualPane))
	{
		AppendItem(menuView, IDM_VIEW_DUAL_PANE, IDS_VIEW_DUAL_PANE);
	}

	AppendItem(menuView, IDM_VIEW_STATUSBAR, IDS_VIEW_STATUSBAR);
	AppendItem(menuView, IDM_VIEW_FOLDERS, IDS_VIEW_FOLDERS);
	AppendItem(menuView, IDM_VIEW_DISPLAYWINDOW, IDS_VIEW_DISPLAYWINDOW);

	auto *toolbarsMenuView = AppendSubMenu(menuView, IDS_VIEW_TOOLBARS_POPUP);
	m_childMenus.push_back(std::make_unique<ToolbarOptionsMenu>(toolbarsMenuView,
		ToolbarOptionsMenu::Source::MainMenu, browser, appServices));

	menuView->AppendSeparator();

	AppendItem(menuView, IDM_VIEW_DECREASE_TEXT_SIZE, IDS_VIEW_DECREASE_TEXT_SIZE);
	AppendItem(menuView, IDM_VIEW_INCREASE_TEXT_SIZE, IDS_VIEW_INCREASE_TEXT_SIZE);

	menuView->AppendSeparator();

	m_childMenus.push_back(
		std::make_unique<ViewsMenu>(menuView, m_acceleratorManager, browser, m_resourceLoader));

	menuView->AppendSeparator();

	AppendItem(menuView, IDM_VIEW_AUTOARRANGE, IDS_VIEW_AUTOARRANGE);

	menuView->AppendSeparator();

	auto *sortByMenuView = AppendSubMenu(menuView, IDS_VIEW_SORTBY);
	m_childMenus.push_back(std::make_unique<SortGroupMenu>(sortByMenuView, m_acceleratorManager,
		browser, SortGroupMenu::Mode::Sort, m_resourceLoader));

	auto *groupByMenuView =
		menuView->AppendSubMenu(SubMenuItemParams(m_resourceLoader->LoadString(IDS_VIEW_GROUPBY))
				.SetEnabledCallback(
					[browser] { return browser->GetActiveShellBrowser()->CanEnableGroupMode(); }));
	m_childMenus.push_back(std::make_unique<SortGroupMenu>(groupByMenuView, m_acceleratorManager,
		browser, SortGroupMenu::Mode::Group, m_resourceLoader));

	AppendItem(menuView, IDM_VIEW_SHOWHIDDENFILES, IDS_VIEW_SHOWHIDDENFILES);
	AppendItem(menuView, IDM_VIEW_REFRESH, IDS_VIEW_REFRESH);
	AppendItem(menuView, IDM_VIEW_SELECTCOLUMNS, IDS_VIEW_SELECTCOLUMNS);
	AppendItem(menuView, IDM_VIEW_AUTOSIZECOLUMNS, IDS_VIEW_AUTOSIZECOLUMNS);
	AppendItem(menuView, IDM_VIEW_SAVECOLUMNLAYOUTASDEFAULT, IDS_VIEW_SAVECOLUMNLAYOUTASDEFAULT);
	AppendItem(menuView, IDM_VIEW_CHANGEDISPLAYCOLOURS, IDS_VIEW_CHANGEDISPLAYCOLOURS);

	menuView->AppendSeparator();

	auto *filterMenuView = AppendSubMenu(menuView, IDS_VIEW_FILTER_POPUP);
	BuildFilterSubMenu(filterMenuView);
}

void MainMenu::BuildFilterSubMenu(MenuView *filterMenuView)
{
	AppendItem(filterMenuView, IDM_FILTER_FILTERRESULTS, IDS_FILTER_FILTERRESULTS);
	AppendItem(filterMenuView, IDM_FILTER_ENABLE_FILTER, IDS_FILTER_ENABLE_FILTER);
}

void MainMenu::BuildActionsMenu()
{
	auto *menuView = AppendSubMenu(m_rootMenuView, IDS_ACTIONS_POPUP);

	AppendItem(menuView, IDM_ACTIONS_NEWFOLDER, IDS_ACTIONS_NEWFOLDER);

	menuView->AppendSeparator();

	AppendItem(menuView, IDM_ACTIONS_SPLITFILE, IDS_ACTIONS_SPLITFILE);
	AppendItem(menuView, IDM_ACTIONS_MERGEFILES, IDS_ACTIONS_MERGEFILES);
	AppendItem(menuView, IDM_ACTIONS_DESTROYFILES, IDS_ACTIONS_DESTROYFILES);
}

void MainMenu::BuildGoMenu(BrowserWindow *browser, AppServices *appServices,
	ShellIconLoader *shellIconLoader)
{
	auto *menuView = AppendSubMenu(m_rootMenuView, IDS_GO_POPUP);

	AppendItem(menuView, IDM_GO_BACK, IDS_GO_BACK);
	AppendItem(menuView, IDM_GO_FORWARD, IDS_GO_FORWARD);
	AppendItem(menuView, IDM_GO_UP, IDS_GO_UP);

	menuView->AppendSeparator();

	auto *historySubMenuView = AppendSubMenu(menuView, IDS_GO_HISTORY_POPUP);
	m_childMenus.push_back(std::make_unique<HistoryMenu>(historySubMenuView, m_acceleratorManager,
		appServices->GetHistoryModel(), browser, shellIconLoader));

	auto *frequentLocationsSubMenuView = AppendSubMenu(menuView, IDS_GO_FREQUENT_LOCATIONS_POPUP);
	m_childMenus.push_back(std::make_unique<FrequentLocationsMenu>(frequentLocationsSubMenuView,
		m_acceleratorManager, appServices->GetFrequentLocationsModel(), browser, shellIconLoader));

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

void MainMenu::AppendGoMenuItem(MenuView *goMenuView, WORD id, const KNOWNFOLDERID &folderId,
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

void MainMenu::AppendGoMenuItem(MenuView *goMenuView, WORD id, const std::wstring &path,
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

void MainMenu::AppendGoMenuItem(MenuView *goMenuView, WORD id, const PidlAbsolute &pidl,
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

	goMenuView->AppendItem(MenuItemParams(folderName)
			.SetIconModel(std::make_unique<ShellIconModel>(shellIconLoader, pidl.Raw()))
			.SetHelpText(folderPath)
			.SetSelectedCallback([this, id] { SendCommandToBrowser(id); }));
}

void MainMenu::BuildBookmarksMenu(BrowserWindow *browser, AppServices *appServices,
	IconFetcher *iconFetcher)
{
	auto *menuView = AppendSubMenu(m_rootMenuView, IDS_BOOKMARKS_POPUP);

	m_childMenus.push_back(std::make_unique<BookmarksMainMenu>(menuView, m_acceleratorManager,
		appServices->GetBookmarkTree(), browser, iconFetcher, appServices->GetPlatformContext(),
		m_resourceLoader));
}

void MainMenu::BuildToolsMenu(BrowserWindow *browser, const FeatureList *featureList,
	Plugins::PluginMenuItemsRegistry *pluginMenuItemsRegistry)
{
	auto *menuView = AppendSubMenu(m_rootMenuView, IDS_TOOLS_POPUP);

	m_childMenus.push_back(std::make_unique<ToolsMenu>(menuView, m_acceleratorManager, browser,
		featureList, pluginMenuItemsRegistry, m_resourceLoader));
}

void MainMenu::BuildWindowMenu()
{
	auto *menuView = AppendSubMenu(m_rootMenuView, IDS_WINDOW_POPUP);

	AppendItem(menuView, IDM_WINDOW_SEARCH_TABS, IDS_WINDOW_SEARCH_TABS);
}

void MainMenu::BuildHelpMenu()
{
	auto *menuView = AppendSubMenu(m_rootMenuView, IDS_HELP_POPUP);

	AppendItem(menuView, IDM_HELP_ONLINE_DOCUMENTATION, IDS_HELP_ONLINE_DOCUMENTATION);

	menuView->AppendSeparator();

	AppendItem(menuView, IDM_HELP_CHECKFORUPDATES, IDS_HELP_CHECKFORUPDATES);
	AppendItem(menuView, IDM_HELP_ABOUT, IDS_HELP_ABOUT);
}

void MainMenu::AppendItem(MenuView *menuView, WORD id, UINT textResourceId)
{
	std::wstring helpText;

	if (auto helpTextId = MaybeGetHelpTextIdForCommand(id))
	{
		helpText = m_resourceLoader->LoadString(*helpTextId);
	}

	menuView->AppendItem(MenuItemParams(m_resourceLoader->LoadString(textResourceId))
			.SetIconModel(MaybeCreateIconModel(id))
			.SetHelpText(helpText)
			.SetAcceleratorText(GetAcceleratorTextForId(id))
			.SetEnabledCallback(MaybeGetEnabledCallback(id))
			.SetCheckedCallback(MaybeGetCheckedCallback(id))
			.SetSelectedCallback([this, id] { SendCommandToBrowser(id); }));
}

MenuView *MainMenu::AppendSubMenu(MenuView *menuView, UINT textResourceId)
{
	return menuView->AppendSubMenu(m_resourceLoader->LoadString(textResourceId));
}

std::unique_ptr<IconModel> MainMenu::MaybeCreateIconModel(WORD id) const
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

std::optional<Icon> MainMenu::GetIconForCommand(WORD id)
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

	case IDM_HELP_ONLINE_DOCUMENTATION:
		return Icon::Help;
	}

	return std::nullopt;
}

std::optional<UINT> MainMenu::MaybeGetHelpTextIdForCommand(WORD id)
{
	switch (id)
	{
	case IDM_FILE_NEWTAB:
		return IDS_FILE_NEWTAB_HELP_TEXT;

	case IDM_FILE_CLOSETAB:
		return IDS_FILE_CLOSETAB_HELP_TEXT;

	case IDM_FILE_NEW_WINDOW:
		return IDS_FILE_NEW_WINDOW_HELP_TEXT;

	case IDM_FILE_CLONEWINDOW:
		return IDS_FILE_CLONEWINDOW_HELP_TEXT;

	case IDM_FILE_SAVEDIRECTORYLISTING:
		return IDS_FILE_SAVEDIRECTORYLISTING_HELP_TEXT;

	case IDM_FILE_OPENCOMMANDPROMPT:
		return IDS_FILE_OPENCOMMANDPROMPT_HELP_TEXT;

	case IDM_FILE_OPENCOMMANDPROMPTADMINISTRATOR:
		return IDS_FILE_OPENCOMMANDPROMPTADMINISTRATOR_HELP_TEXT;

	case IDM_FILE_COPYFOLDERPATH:
		return IDS_FILE_COPYFOLDERPATH_HELP_TEXT;

	case IDM_FILE_COPYITEMPATH:
		return IDS_FILE_COPYITEMPATH_HELP_TEXT;

	case IDM_FILE_COPYUNIVERSALFILEPATHS:
		return IDS_FILE_COPYUNIVERSALFILEPATHS_HELP_TEXT;

	case IDM_FILE_COPYCOLUMNTEXT:
		return IDS_FILE_COPYCOLUMNTEXT_HELP_TEXT;

	case IDM_FILE_SETFILEATTRIBUTES:
		return IDS_FILE_SETFILEATTRIBUTES_HELP_TEXT;

	case IDM_FILE_DELETE:
		return IDS_FILE_DELETE_HELP_TEXT;

	case IDM_FILE_DELETEPERMANENTLY:
		return IDS_FILE_DELETEPERMANENTLY_HELP_TEXT;

	case IDM_FILE_RENAME:
		return IDS_FILE_RENAME_HELP_TEXT;

	case IDM_FILE_PROPERTIES:
		return IDS_FILE_PROPERTIES_HELP_TEXT;

	case IDM_FILE_EXIT:
		return IDS_FILE_EXIT_HELP_TEXT;

	case IDM_EDIT_UNDO:
		return IDS_EDIT_UNDO_HELP_TEXT;

	case IDM_EDIT_CUT:
		return IDS_EDIT_CUT_HELP_TEXT;

	case IDM_EDIT_COPY:
		return IDS_EDIT_COPY_HELP_TEXT;

	case IDM_EDIT_PASTE:
		return IDS_EDIT_PASTE_HELP_TEXT;

	case IDM_EDIT_PASTESHORTCUT:
		return IDS_EDIT_PASTESHORTCUT_HELP_TEXT;

	case IDM_EDIT_PASTEHARDLINK:
		return IDS_EDIT_PASTEHARDLINK_HELP_TEXT;

	case IDM_EDIT_PASTE_SYMBOLIC_LINK:
		return IDS_EDIT_PASTE_SYMBOLIC_LINK_HELP_TEXT;

	case IDM_EDIT_COPYTOFOLDER:
		return IDS_EDIT_COPYTOFOLDER_HELP_TEXT;

	case IDM_EDIT_MOVETOFOLDER:
		return IDS_EDIT_MOVETOFOLDER_HELP_TEXT;

	case IDM_EDIT_RESOLVELINK:
		return IDS_EDIT_RESOLVELINK_HELP_TEXT;

	case IDM_EDIT_SELECTALL:
		return IDS_EDIT_SELECTALL_HELP_TEXT;

	case IDM_EDIT_INVERTSELECTION:
		return IDS_EDIT_INVERTSELECTION_HELP_TEXT;

	case IDM_EDIT_SELECTALLOFSAMETYPE:
		return IDS_EDIT_SELECTALLOFSAMETYPE_HELP_TEXT;

	case IDM_EDIT_SELECTNONE:
		return IDS_EDIT_SELECTNONE_HELP_TEXT;

	case IDM_EDIT_WILDCARDSELECTION:
		return IDS_EDIT_WILDCARDSELECTION_HELP_TEXT;

	case IDM_EDIT_WILDCARDDESELECT:
		return IDS_EDIT_WILDCARDDESELECT_HELP_TEXT;

	case IDM_VIEW_DUAL_PANE:
		return IDS_VIEW_DUAL_PANE_HELP_TEXT;

	case IDM_VIEW_STATUSBAR:
		return IDS_VIEW_STATUSBAR_HELP_TEXT;

	case IDM_VIEW_FOLDERS:
		return IDS_VIEW_FOLDERS_HELP_TEXT;

	case IDM_VIEW_DISPLAYWINDOW:
		return IDS_VIEW_DISPLAYWINDOW_HELP_TEXT;

	case IDM_VIEW_DECREASE_TEXT_SIZE:
		return IDS_VIEW_DECREASE_TEXT_SIZE_HELP_TEXT;

	case IDM_VIEW_INCREASE_TEXT_SIZE:
		return IDS_VIEW_INCREASE_TEXT_SIZE_HELP_TEXT;

	case IDM_VIEW_AUTOARRANGE:
		return IDS_VIEW_AUTOARRANGE_HELP_TEXT;

	case IDM_VIEW_SHOWHIDDENFILES:
		return IDS_VIEW_SHOWHIDDENFILES_HELP_TEXT;

	case IDM_VIEW_REFRESH:
		return IDS_VIEW_REFRESH_HELP_TEXT;

	case IDM_VIEW_SELECTCOLUMNS:
		return IDS_VIEW_SELECTCOLUMNS_HELP_TEXT;

	case IDM_VIEW_SAVECOLUMNLAYOUTASDEFAULT:
		return IDS_VIEW_SAVECOLUMNLAYOUTASDEFAULT_HELP_TEXT;

	case IDM_VIEW_CHANGEDISPLAYCOLOURS:
		return IDS_VIEW_CHANGEDISPLAYCOLOURS_HELP_TEXT;

	case IDM_FILTER_FILTERRESULTS:
		return IDS_FILTER_FILTERRESULTS_HELP_TEXT;

	case IDM_FILTER_ENABLE_FILTER:
		return IDS_FILTER_ENABLE_FILTER_HELP_TEXT;

	case IDM_ACTIONS_NEWFOLDER:
		return IDS_ACTIONS_NEWFOLDER_HELP_TEXT;

	case IDM_ACTIONS_SPLITFILE:
		return IDS_ACTIONS_SPLITFILE_HELP_TEXT;

	case IDM_ACTIONS_MERGEFILES:
		return IDS_ACTIONS_MERGEFILES_HELP_TEXT;

	case IDM_ACTIONS_DESTROYFILES:
		return IDS_ACTIONS_DESTROYFILES_HELP_TEXT;

	case IDM_GO_BACK:
		return IDS_GO_BACK_HELP_TEXT;

	case IDM_GO_FORWARD:
		return IDS_GO_FORWARD_HELP_TEXT;

	case IDM_GO_UP:
		return IDS_GO_UP_HELP_TEXT;

	case IDM_WINDOW_SEARCH_TABS:
		return IDS_WINDOW_SEARCH_TABS_HELP_TEXT;

	case IDM_HELP_ONLINE_DOCUMENTATION:
		return IDS_HELP_ONLINE_DOCUMENTATION_HELP_TEXT;

	case IDM_HELP_CHECKFORUPDATES:
		return IDS_HELP_CHECKFORUPDATES_HELP_TEXT;

	case IDM_HELP_ABOUT:
		return IDS_HELP_ABOUT_HELP_TEXT;
	}

	return std::nullopt;
}

MenuItemEnabledCallback MainMenu::MaybeGetEnabledCallback(WORD id) const
{
	switch (id)
	{
	case IDM_FILE_CLOSETAB:
	case IDM_FILE_SAVEDIRECTORYLISTING:
	case IDM_FILE_OPENCOMMANDPROMPT:
	case IDM_FILE_OPENCOMMANDPROMPTADMINISTRATOR:
	case IDM_FILE_COPYITEMPATH:
	case IDM_FILE_COPYUNIVERSALFILEPATHS:
	case IDM_FILE_SETFILEATTRIBUTES:
	case IDM_FILE_DELETE:
	case IDM_FILE_DELETEPERMANENTLY:
	case IDM_FILE_RENAME:
	case IDM_FILE_PROPERTIES:
	case IDM_EDIT_CUT:
	case IDM_EDIT_COPY:
	case IDM_EDIT_COPYTOFOLDER:
	case IDM_EDIT_MOVETOFOLDER:
	case IDM_EDIT_SELECTNONE:
	case IDM_EDIT_WILDCARDDESELECT:
	case IDM_VIEW_DECREASE_TEXT_SIZE:
	case IDM_VIEW_INCREASE_TEXT_SIZE:
	case IDM_VIEW_AUTOARRANGE:
	case IDM_VIEW_AUTOSIZECOLUMNS:
	case IDM_ACTIONS_NEWFOLDER:
	case IDM_ACTIONS_SPLITFILE:
	case IDM_ACTIONS_MERGEFILES:
	case IDM_GO_BACK:
	case IDM_GO_FORWARD:
	case IDM_GO_UP:
		return [this, id]
		{
			return m_browser->GetCommandController()->IsCommandEnabled(id);
		};

	case IDM_FILE_COPYCOLUMNTEXT:
		return [this]
		{
			auto *shellBrowser =
				m_browser->GetActiveTabContainer()->GetSelectedTab().GetShellBrowserImpl();

			return shellBrowser->GetNumSelected() > 0
				&& (shellBrowser->GetViewMode() == ViewMode::Details);
		};

	case IDM_EDIT_UNDO:
		return [this]
		{
			return m_fileActionHandler->CanUndo();
		};

	case IDM_EDIT_PASTE:
		return [this]
		{
			return m_coreInterface->CanPaste(PasteType::Normal);
		};

	case IDM_EDIT_PASTESHORTCUT:
		return [this]
		{
			return m_coreInterface->CanPaste(PasteType::Shortcut);
		};

	case IDM_EDIT_PASTEHARDLINK:
	case IDM_EDIT_PASTE_SYMBOLIC_LINK:
		return [this]
		{
			const auto *shellBrowser = m_browser->GetActiveShellBrowser();
			return ClipboardOperations::CanPasteLinkInDirectory(m_clipboardStore,
				shellBrowser->GetDirectory().Raw());
		};

	case IDM_EDIT_RESOLVELINK:
	case IDM_ACTIONS_DESTROYFILES:
		return [this]
		{
			return m_browser->GetActiveTabContainer()
					   ->GetSelectedTab()
					   .GetShellBrowserImpl()
					   ->GetNumSelected()
				> 0;
		};
	}

	return {};
}

MenuItemCheckedCallback MainMenu::MaybeGetCheckedCallback(WORD id) const
{
	switch (id)
	{
	case IDM_VIEW_DUAL_PANE:
		return [this]
		{
			return m_config->dualPane;
		};

	case IDM_VIEW_STATUSBAR:
		return [this]
		{
			return m_config->showStatusBar.get();
		};

	case IDM_VIEW_FOLDERS:
		return [this]
		{
			return m_config->showFolders.get();
		};

	case IDM_VIEW_DISPLAYWINDOW:
		return [this]
		{
			return m_config->showDisplayWindow.get();
		};

	case IDM_VIEW_AUTOARRANGE:
		return [this]
		{
			return m_browser->GetCommandController()->IsCommandEnabled(IDM_VIEW_AUTOARRANGE)
				&& m_browser->GetActiveShellBrowser()->IsAutoArrangeEnabled();
		};

	case IDM_VIEW_SHOWHIDDENFILES:
		return [this]
		{
			return m_browser->GetActiveShellBrowser()->GetFolderSettings().showHidden;
		};

	case IDM_FILTER_ENABLE_FILTER:
		return [this]
		{
			return m_browser->GetActiveShellBrowser()->IsFilterEnabled();
		};
	}

	return {};
}

void MainMenu::SendCommandToBrowser(WORD id)
{
	SendMessage(m_browser->GetHWND(), WM_COMMAND, MAKEWPARAM(id, 0), 0);
}
