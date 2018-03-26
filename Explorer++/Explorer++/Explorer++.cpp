/******************************************************************
 *
 * Project: Explorer++
 * File: Explorer++.cpp
 * License: GPL - See LICENSE in the top level directory
 *
 * Main file for the Explorerplusplus class.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "DefaultColumns.h"
#include "iServiceProvider.h"
#include "../Helper/ShellHelper.h"


CRITICAL_SECTION	g_csDirMonCallback;

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

Explorerplusplus::Explorerplusplus(HWND hwnd) :
m_hContainer(hwnd)
{
	m_hLanguageModule				= nullptr;

	/* When the 'open new tabs next to
	current' option is activated, the
	first tab will open at the index
	m_iTabSelectedItem + 1 - therefore
	this variable must be initialized. */
	m_selectedTabIndex				= 0;

	/* Initial state. */
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
	m_bTabBeenDragged				= FALSE;
	m_bTreeViewDelayEnabled			= FALSE;
	m_bSavePreferencesToXMLFile		= FALSE;
	m_bAttemptToolbarRestore		= FALSE;
	m_bLanguageLoaded				= FALSE;
	m_bListViewRenaming				= FALSE;
	m_bDragging						= FALSE;
	m_bDragCancelled				= FALSE;
	m_bDragAllowed					= FALSE;
	m_pActiveShellBrowser			= NULL;
	m_hMainRebar					= NULL;
	m_hStatusBar					= NULL;
	m_hHolder						= NULL;
	m_hTabBacking					= NULL;
	m_hTabCtrl						= NULL;
	m_hTabWindowToolbar				= NULL;
	m_hDisplayWindow				= NULL;
	m_hTreeView						= NULL;
	m_hFoldersToolbar				= NULL;
	m_hLastActiveWindow				= NULL;
	m_hActiveListView				= NULL;
	m_hTabFont						= NULL;
	m_hTabCtrlImageList				= nullptr;
	m_hNextClipboardViewer			= NULL;
	m_ListViewMButtonItem			= -1;
	m_zDeltaTotal					= 0;
	m_iPreviousTabSelectionId		= -1;
	m_InitializationFinished		= false;

	m_pTaskbarList					= NULL;

	m_bBlockNext = FALSE;

	SetLanguageModule();

	m_ColorRules = NColorRuleHelper::GetDefaultColorRules(m_hLanguageModule);

	SetDefaultValues();
	SetDefaultColumns();

	InitializeTabMap();

	/* Default folder (i.e. My Computer). */
	GetCsidlDisplayName(CSIDL_DRIVES,m_DefaultTabDirectoryStatic,SIZEOF_ARRAY(m_DefaultTabDirectoryStatic),SHGDN_FORPARSING);
	GetCsidlDisplayName(CSIDL_DRIVES,m_DefaultTabDirectory,SIZEOF_ARRAY(m_DefaultTabDirectory),SHGDN_FORPARSING);

	InitializeMainToolbars();

	InitializeCriticalSection(&g_csDirMonCallback);

	m_iDWFolderSizeUniqueId = 0;

	m_pClipboardDataObject	= NULL;
	m_iCutTabInternal		= 0;
	m_hCutTreeViewItem		= NULL;

	m_ViewModes.push_back(VM_EXTRALARGEICONS);
	m_ViewModes.push_back(VM_LARGEICONS);
	m_ViewModes.push_back(VM_ICONS);
	m_ViewModes.push_back(VM_SMALLICONS);
	m_ViewModes.push_back(VM_LIST);
	m_ViewModes.push_back(VM_DETAILS);
	m_ViewModes.push_back(VM_THUMBNAILS);
	m_ViewModes.push_back(VM_TILES);
}

Explorerplusplus::~Explorerplusplus()
{
	if(m_hTabFont != NULL)
	{
		DeleteObject(m_hTabFont);
	}

	if (m_hTabCtrlImageList != nullptr)
	{
		ImageList_Destroy(m_hTabCtrlImageList);
	}

	delete m_pTabContainer;

	/* Bookmarks teardown. */
	delete m_pBookmarksToolbar;

	m_pDirMon->Release();
}

void Explorerplusplus::SetDefaultValues(void)
{
	/* User options. */
	m_bOpenNewTabNextToCurrent		= FALSE;
	m_bConfirmCloseTabs				= FALSE;
	m_bShowFullTitlePath			= FALSE;
	m_bAlwaysOpenNewTab				= FALSE;
	m_bShowFolderSizes				= FALSE;
	m_bDisableFolderSizesNetworkRemovable	 = FALSE;
	m_StartupMode					= STARTUP_PREVIOUSTABS;
	m_bExtendTabControl				= FALSE;
	m_bShowUserNameInTitleBar		= FALSE;
	m_bShowPrivilegeLevelInTitleBar	= FALSE;
	m_bShowFilePreviews				= TRUE;
	m_ReplaceExplorerMode			= NDefaultFileManager::REPLACEEXPLORER_NONE;
	m_bOneClickActivate				= FALSE;
	m_OneClickActivateHoverTime		= DEFAULT_LISTVIEW_HOVER_TIME;
	m_bAllowMultipleInstances		= TRUE;
	m_bForceSameTabWidth			= FALSE;
	m_bDoubleClickTabClose			= TRUE;
	m_bHandleZipFiles				= FALSE;
	m_bInsertSorted					= TRUE;
	m_bOverwriteExistingFilesConfirmation	= TRUE;
	m_bCheckBoxSelection			= FALSE;
	m_bForceSize					= FALSE;
	m_SizeDisplayFormat				= SIZE_FORMAT_BYTES;
	m_bSynchronizeTreeview			= TRUE;
	m_bTVAutoExpandSelected			= FALSE;
	m_bCloseMainWindowOnTabClose	= TRUE;
	m_bLargeToolbarIcons			= FALSE;
	m_bShowTaskbarThumbnails		= TRUE;
	m_bPlayNavigationSound			= TRUE;

	/* Infotips (user options). */
	m_bShowInfoTips					= TRUE;
	m_InfoTipType					= INFOTIP_SYSTEM;

	/* Window states. */
	m_bShowStatusBar				= TRUE;
	m_bShowFolders					= TRUE;
	m_bShowAddressBar				= TRUE;
	m_bShowMainToolbar				= TRUE;
	m_bShowBookmarksToolbar			= FALSE;
	m_bShowDisplayWindow			= TRUE;
	m_bShowDrivesToolbar			= TRUE;
	m_bShowApplicationToolbar		= FALSE;
	m_bAlwaysShowTabBar				= TRUE;
	m_bShowTabBar					= TRUE;
	m_bLockToolbars					= TRUE;
	m_DisplayWindowHeight			= DEFAULT_DISPLAYWINDOW_HEIGHT;
	m_TreeViewWidth					= DEFAULT_TREEVIEW_WIDTH;
	m_bUseFullRowSelect				= FALSE;
	m_bShowTabBarAtBottom			= FALSE;

	/* Global options. */
	m_ViewModeGlobal				= VM_ICONS;
	m_bShowHiddenGlobal				= TRUE;
	m_bShowExtensionsGlobal			= TRUE;
	m_bShowInGroupsGlobal			= FALSE;
	m_bAutoArrangeGlobal			= TRUE;
	m_bSortAscendingGlobal			= TRUE;
	m_bShowGridlinesGlobal			= TRUE;
	m_bShowFriendlyDatesGlobal		= TRUE;
	m_bHideSystemFilesGlobal		= FALSE;
	m_bHideLinkExtensionGlobal		= FALSE;
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