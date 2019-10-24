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
m_acceleratorUpdater(&g_hAccl),
m_pluginCommandManager(&g_hAccl, ACCELERATOR_PLUGIN_STARTID, ACCELERATOR_PLUGIN_ENDID)
{
	m_hLanguageModule				= nullptr;

	m_config = std::make_shared<Config>();

	/* Initial state. */
	m_nSelected						= 0;
	m_nSelectedOnInvert				= 0;
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
	m_InitializationFinished		= false;

	m_bBlockNext = FALSE;

	m_ColorRules = NColorRuleHelper::GetDefaultColorRules();

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