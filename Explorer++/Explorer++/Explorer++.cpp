/******************************************************************
 *
 * Project: Explorer++
 * File: Explorer++.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Main file for the Explorerplusplus class.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "iServiceProvider.h"
#include "../Helper/ShellHelper.h"


CRITICAL_SECTION	g_csDirMonCallback;

Explorerplusplus::Explorerplusplus(HWND hwnd) :
m_hContainer(hwnd)
{
	/* When the 'open new tabs next to
	current' option is activated, the
	first tab will open at the index
	m_iTabSelectedItem + 1 - therefore
	this variable must be initialized. */
	m_iTabSelectedItem				= 0;

	/* Initial state. */
	m_nSelected						= 0;
	m_iObjectIndex					= 0;
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
	g_hwndSearch					= NULL;
	g_hwndOptions					= NULL;
	g_hwndManageBookmarks			= NULL;
	m_ListViewMButtonItem			= -1;
	m_zDeltaTotal					= 0;
	m_iPreviousTabSelectionId		= -1;

	m_pTaskbarList3					= NULL;

	m_bBlockNext = FALSE;

	m_ColorRules = NColorRuleHelper::GetDefaultColorRules();

	SetDefaultValues();
	SetAllDefaultColumns();

	InitializeTabMap();

	/* Default folder (i.e. My Computer). */
	GetVirtualFolderParsingPath(CSIDL_DRIVES,m_DefaultTabDirectoryStatic);
	GetVirtualFolderParsingPath(CSIDL_DRIVES,m_DefaultTabDirectory);

	InitializeMainToolbars();
	InitializeApplicationToolbar();

	InitializeCriticalSection(&g_csDirMonCallback);

	m_iDWFolderSizeUniqueId = 0;

	m_pClipboardDataObject	= NULL;
	m_iCutTabInternal		= 0;
	m_hCutTreeViewItem		= NULL;

	/* View modes. */
	OSVERSIONINFO VersionInfo;
	ViewMode_t ViewMode;

	VersionInfo.dwOSVersionInfoSize	= sizeof(OSVERSIONINFO);

	if(GetVersionEx(&VersionInfo) != 0)
	{
		m_dwMajorVersion = VersionInfo.dwMajorVersion;
		m_dwMinorVersion = VersionInfo.dwMinorVersion;

		if(VersionInfo.dwMajorVersion >= WINDOWS_VISTA_SEVEN_MAJORVERSION)
		{
			ViewMode.uViewMode = VM_EXTRALARGEICONS;
			m_ViewModes.push_back(ViewMode);

			ViewMode.uViewMode = VM_LARGEICONS;
			m_ViewModes.push_back(ViewMode);

			ViewMode.uViewMode = VM_ICONS;
			m_ViewModes.push_back(ViewMode);

			ViewMode.uViewMode = VM_SMALLICONS;
			m_ViewModes.push_back(ViewMode);

			ViewMode.uViewMode = VM_LIST;
			m_ViewModes.push_back(ViewMode);

			ViewMode.uViewMode = VM_DETAILS;
			m_ViewModes.push_back(ViewMode);

			ViewMode.uViewMode = VM_THUMBNAILS;
			m_ViewModes.push_back(ViewMode);

			ViewMode.uViewMode = VM_TILES;
			m_ViewModes.push_back(ViewMode);
		}
		else
		{
			ViewMode.uViewMode = VM_THUMBNAILS;
			m_ViewModes.push_back(ViewMode);

			ViewMode.uViewMode = VM_TILES;
			m_ViewModes.push_back(ViewMode);

			ViewMode.uViewMode = VM_ICONS;
			m_ViewModes.push_back(ViewMode);

			ViewMode.uViewMode = VM_SMALLICONS;
			m_ViewModes.push_back(ViewMode);

			ViewMode.uViewMode = VM_LIST;
			m_ViewModes.push_back(ViewMode);

			ViewMode.uViewMode = VM_DETAILS;
			m_ViewModes.push_back(ViewMode);
		}
	}

	m_hDwmapi = LoadLibrary(_T("dwmapi.dll"));

	if(m_hDwmapi != NULL)
	{
		DwmInvalidateIconicBitmaps = (DwmInvalidateIconicBitmapsProc)GetProcAddress(m_hDwmapi,"DwmInvalidateIconicBitmaps");
	}
	else
	{
		DwmInvalidateIconicBitmaps = NULL;
	}
}

Explorerplusplus::~Explorerplusplus()
{
	/* Bookmarks teardown. */
	delete m_pipbin;
	delete m_pBookmarksToolbar;

	m_pDirMon->Release();

	if(m_hDwmapi != NULL)
	{
		FreeLibrary(m_hDwmapi);
	}
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