// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "App.h"
#include "Application.h"
#include "Bookmarks/BookmarkIconManager.h"
#include "Bookmarks/UI/BookmarksMainMenu.h"
#include "Config.h"
#include "Explorer++_internal.h"
#include "FeatureList.h"
#include "GlobalHistoryMenu.h"
#include "HistoryServiceFactory.h"
#include "MainFontSetter.h"
#include "MainMenuSubMenuView.h"
#include "MainRebarStorage.h"
#include "MenuRanges.h"
#include "Plugins/PluginManager.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowserHistoryHelper.h"
#include "TabRestorer.h"
#include "TabRestorerMenu.h"
#include "TabStorage.h"
#include "ThemeWindowTracker.h"
#include "UiTheming.h"
#include "../Helper/WindowSubclassWrapper.h"
#include "../Helper/iDirectoryMonitor.h"

Explorerplusplus::Explorerplusplus(App *app, HWND hwnd) :
	m_app(app),
	m_hContainer(hwnd),
	m_commandController(this),
	m_tabBarBackgroundBrush(CreateSolidBrush(TAB_BAR_DARK_MODE_BACKGROUND_COLOR)),
	m_pluginMenuManager(hwnd, MENU_PLUGIN_START_ID, MENU_PLUGIN_END_ID),
	m_acceleratorUpdater(app->GetAcceleratorManager()),
	m_pluginCommandManager(app->GetAcceleratorManager(), ACCELERATOR_PLUGIN_START_ID,
		ACCELERATOR_PLUGIN_END_ID),
	m_config(app->GetConfig()),
	m_iconFetcher(hwnd, m_app->GetCachedIcons()),
	m_shellIconLoader(&m_iconFetcher)
{
	m_resourceInstance = nullptr;

	FeatureList::GetInstance()->InitializeFromCommandLine(*app->GetCommandLineSettings());

	m_bSavePreferencesToXMLFile = FALSE;
	m_bLanguageLoaded = false;
	m_bShowTabBar = true;
	m_pActiveShellBrowser = nullptr;
	m_hMainRebar = nullptr;
	m_hStatusBar = nullptr;
	m_hTabBacking = nullptr;
	m_hTabWindowToolbar = nullptr;
	m_hDisplayWindow = nullptr;
	m_lastActiveWindow = nullptr;
	m_hActiveListView = nullptr;

	m_iDWFolderSizeUniqueId = 0;
}

Explorerplusplus::~Explorerplusplus()
{
	m_pDirMon->Release();
}

BrowserCommandController *Explorerplusplus::GetCommandController()
{
	return &m_commandController;
}

BrowserPane *Explorerplusplus::GetActivePane() const
{
	return m_browserPane.get();
}

ShellBrowser *Explorerplusplus::GetActiveShellBrowser()
{
	return GetActivePane()->GetTabContainer()->GetSelectedTab().GetShellBrowser();
}

void Explorerplusplus::OnShellBrowserCreated(ShellBrowser *shellBrowser)
{
	ShellBrowserHistoryHelper::CreateAndAttachToShellBrowser(shellBrowser,
		HistoryServiceFactory::GetInstance()->GetHistoryService());
}
