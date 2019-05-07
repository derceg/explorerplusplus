// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "BookmarksToolbar.h"
#include "ColorRuleHelper.h"
#include "Config.h"
#include "DefaultColumns.h"
#include "Explorer++_internal.h"
#include "iServiceProvider.h"
#include "MenuRanges.h"
#include "PluginManager.h"
#include "ShellBrowser/ViewModes.h"
#include "../Helper/iDirectoryMonitor.h"
#include "../Helper/ShellHelper.h"

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

const std::array<ViewMode, 8> Explorerplusplus::m_viewModes = {
	ViewMode::ExtraLargeIcons,
	ViewMode::LargeIcons,
	ViewMode::Icons,
	ViewMode::SmallIcons,
	ViewMode::List,
	ViewMode::Details,
	ViewMode::Thumbnails,
	ViewMode::Tiles
};

Explorerplusplus::Explorerplusplus(HWND hwnd) :
m_hContainer(hwnd),
m_pluginMenuManager(hwnd, MENU_PLUGIN_STARTID, MENU_PLUGIN_ENDID),
m_pluginCommandManager(&g_hAccl, ACCELERATOR_PLUGIN_STARTID, ACCELERATOR_PLUGIN_ENDID),
m_cachedIcons(MAX_CACHED_ICONS)
{
	m_hLanguageModule				= nullptr;

	m_config = std::make_shared<Config>();

	/* When the 'open new tabs next to
	current' option is activated, the
	first tab will open at the index
	m_iTabSelectedItem + 1 - therefore
	this variable must be initialized. */
	m_selectedTabIndex				= 0;

	/* Initial state. */
	m_tabIdCounter					= 1;
	m_nSelected						= 0;
	m_nSelectedOnInvert				= 0;
	m_selectedTabId					= 0;
	m_iMaxArrangeMenuItem			= 0;
	m_bCountingUp					= FALSE;
	m_bCountingDown					= FALSE;
	m_bInverted						= FALSE;
	m_bSelectionFromNowhere			= FALSE;
	m_bSelectingTreeViewDirectory	= FALSE;
	m_bTreeViewRightClick			= FALSE;
	m_bSavePreferencesToXMLFile		= FALSE;
	m_bAttemptToolbarRestore		= FALSE;
	m_bLanguageLoaded				= FALSE;
	m_bListViewRenaming				= FALSE;
	m_bDragging						= FALSE;
	m_bDragCancelled				= FALSE;
	m_bDragAllowed					= FALSE;
	m_bShowTabBar					= TRUE;
	m_pActiveShellBrowser			= NULL;
	m_hMainRebar					= NULL;
	m_hStatusBar					= NULL;
	m_hHolder						= NULL;
	m_hTabBacking					= NULL;
	m_hTabWindowToolbar				= NULL;
	m_hDisplayWindow				= NULL;
	m_hTreeView						= NULL;
	m_hFoldersToolbar				= NULL;
	m_hLastActiveWindow				= NULL;
	m_hActiveListView				= NULL;
	m_hNextClipboardViewer			= NULL;
	m_hLanguageModule				= NULL;
	m_ListViewMButtonItem			= -1;
	m_zDeltaTotal					= 0;
	m_iPreviousTabSelectionId		= -1;
	m_InitializationFinished		= false;

	m_bBlockNext = FALSE;

	SetLanguageModule();

	m_ColorRules = NColorRuleHelper::GetDefaultColorRules(m_hLanguageModule);

	SetDefaultColumns();

	m_iDWFolderSizeUniqueId = 0;

	m_pClipboardDataObject	= NULL;
	m_iCutTabInternal		= 0;
	m_hCutTreeViewItem		= NULL;
}

Explorerplusplus::~Explorerplusplus()
{
	/* Bookmarks teardown. */
	delete m_pBookmarksToolbar;

	m_pDirMon->Release();
}

void Explorerplusplus::SetDefaultColumns()
{
	m_RealFolderColumnList = std::list<Column_t>(REAL_FOLDER_DEFAULT_COLUMNS,
		REAL_FOLDER_DEFAULT_COLUMNS + SIZEOF_ARRAY(REAL_FOLDER_DEFAULT_COLUMNS));
	m_ControlPanelColumnList = std::list<Column_t>(CONTROL_PANEL_DEFAULT_COLUMNS,
		CONTROL_PANEL_DEFAULT_COLUMNS + SIZEOF_ARRAY(CONTROL_PANEL_DEFAULT_COLUMNS));
	m_MyComputerColumnList = std::list<Column_t>(MY_COMPUTER_DEFAULT_COLUMNS,
		MY_COMPUTER_DEFAULT_COLUMNS + SIZEOF_ARRAY(MY_COMPUTER_DEFAULT_COLUMNS));
	m_RecycleBinColumnList = std::list<Column_t>(RECYCLE_BIN_DEFAULT_COLUMNS,
		RECYCLE_BIN_DEFAULT_COLUMNS + SIZEOF_ARRAY(RECYCLE_BIN_DEFAULT_COLUMNS));
	m_PrintersColumnList = std::list<Column_t>(PRINTERS_DEFAULT_COLUMNS,
		PRINTERS_DEFAULT_COLUMNS + SIZEOF_ARRAY(PRINTERS_DEFAULT_COLUMNS));
	m_NetworkConnectionsColumnList = std::list<Column_t>(NETWORK_CONNECTIONS_DEFAULT_COLUMNS,
		NETWORK_CONNECTIONS_DEFAULT_COLUMNS + SIZEOF_ARRAY(NETWORK_CONNECTIONS_DEFAULT_COLUMNS));
	m_MyNetworkPlacesColumnList = std::list<Column_t>(MY_NETWORK_PLACES_DEFAULT_COLUMNS,
		MY_NETWORK_PLACES_DEFAULT_COLUMNS + SIZEOF_ARRAY(MY_NETWORK_PLACES_DEFAULT_COLUMNS));
}