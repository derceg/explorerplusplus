// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Application.h"
#include "Bookmarks/BookmarkIconManager.h"
#include "Bookmarks/UI/BookmarksMainMenu.h"
#include "Config.h"
#include "Explorer++_internal.h"
#include "FeatureList.h"
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

/* These entries correspond to shell
extensions that are known to be
incompatible with Explorer++. They
won't be loaded on any background
context menus. */
const std::vector<std::wstring> Explorerplusplus::BLACKLISTED_BACKGROUND_MENU_CLSID_ENTRIES = {
	/* OneDrive file sync extension
	(see https://github.com/derceg/explorerplusplus/issues/35
	for a description of the issue
	associated with this extension). */
	_T("{CB3D0F55-BC2C-4C1A-85ED-23ED75B5106B}")
};

Explorerplusplus::Explorerplusplus(HWND hwnd, InitializationData *initializationData) :
	m_hContainer(hwnd),
	m_commandLineSettings(initializationData->commandLineSettings),
	m_acceleratorManager(initializationData->acceleratorManager),
	m_commandController(this),
	m_cachedIcons(MAX_CACHED_ICONS),
	m_pluginMenuManager(hwnd, MENU_PLUGIN_START_ID, MENU_PLUGIN_END_ID),
	m_acceleratorUpdater(initializationData->acceleratorManager),
	m_pluginCommandManager(initializationData->acceleratorManager, ACCELERATOR_PLUGIN_START_ID,
		ACCELERATOR_PLUGIN_END_ID),
	m_iconFetcher(hwnd, &m_cachedIcons),
	m_tabBarBackgroundBrush(CreateSolidBrush(TAB_BAR_DARK_MODE_BACKGROUND_COLOR))
{
	m_resourceInstance = nullptr;

	m_config = std::make_shared<Config>();
	FeatureList::GetInstance()->InitializeFromCommandLine(*initializationData->commandLineSettings);

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
	ShellBrowserHistoryHelper::CreateAndAttachToShellBrowser(shellBrowser);
}
