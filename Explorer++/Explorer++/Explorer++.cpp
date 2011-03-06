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
#include "../Helper/ShellHelper.h"
#include "../Helper/SetDefaultFileManager.h"


CRITICAL_SECTION	g_csDirMonCallback;

/* IUnknown interface members. */
HRESULT __stdcall Explorerplusplus::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = NULL;

	if(iid == IID_IServiceProvider)
	{
		*ppvObject = static_cast<IServiceProvider *>(this);
	}

	if(*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall Explorerplusplus::AddRef(void)
{
	return ++m_iRefCount;
}

ULONG __stdcall Explorerplusplus::Release(void)
{
	m_iRefCount--;
	
	if(m_iRefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_iRefCount;
}

/*
 * Constructor for the main Explorerplusplus class.
 */
Explorerplusplus::Explorerplusplus(HWND hwnd)
{
	m_iRefCount = 1;

	m_hContainer					= hwnd;

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
	m_ListViewMButtonItem			= -1;
	m_nDrivesInToolbar				= 0;

	/* Dialog states. */
	m_bAddBookmarkDlgStateSaved		= FALSE;
	m_bDisplayColorsDlgStateSaved	= FALSE;
	m_bOrganizeBookmarksDlgStateSaved	= FALSE;

	m_pTaskbarList3					= NULL;

	m_bBlockNext = FALSE;

	InitializeColorRules();

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

/*
 * Deconstructor for the main Explorerplusplus class.
 */
Explorerplusplus::~Explorerplusplus()
{
	m_pDirMon->Release();

	if(m_hDwmapi != NULL)
	{
		FreeLibrary(m_hDwmapi);
	}
}

void Explorerplusplus::InitializeMainToolbars(void)
{
	/* Initialize the main toolbar styles and settings here. The visibility and gripper
	styles will be set after the settings have been loaded (needed to keep compatibility
	with versions older than 0.9.5.4). */
	m_ToolbarInformation[0].wID			= ID_MAINTOOLBAR;
	m_ToolbarInformation[0].fMask		= RBBIM_ID|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE|RBBIM_IDEALSIZE|RBBIM_STYLE;
	m_ToolbarInformation[0].fStyle		= RBBS_BREAK|RBBS_USECHEVRON;
	m_ToolbarInformation[0].cx			= 0;
	m_ToolbarInformation[0].cxIdeal		= 0;
	m_ToolbarInformation[0].cxMinChild	= 0;
	m_ToolbarInformation[0].cyIntegral	= 0;
	m_ToolbarInformation[0].cxHeader	= 0;
	m_ToolbarInformation[0].lpText		= NULL;

	m_ToolbarInformation[1].wID			= ID_ADDRESSTOOLBAR;
	m_ToolbarInformation[1].fMask		= RBBIM_ID|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE|RBBIM_STYLE|RBBIM_TEXT;
	m_ToolbarInformation[1].fStyle		= RBBS_BREAK;
	m_ToolbarInformation[1].cx			= 0;
	m_ToolbarInformation[1].cxIdeal		= 0;
	m_ToolbarInformation[1].cxMinChild	= 0;
	m_ToolbarInformation[1].cyIntegral	= 0;
	m_ToolbarInformation[1].cxHeader	= 0;
	m_ToolbarInformation[1].lpText		= NULL;

	m_ToolbarInformation[2].wID			= ID_BOOKMARKSTOOLBAR;
	m_ToolbarInformation[2].fMask		= RBBIM_ID|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE|RBBIM_IDEALSIZE|RBBIM_STYLE;
	m_ToolbarInformation[2].fStyle		= RBBS_BREAK|RBBS_USECHEVRON;
	m_ToolbarInformation[2].cx			= 0;
	m_ToolbarInformation[2].cxIdeal		= 0;
	m_ToolbarInformation[2].cxMinChild	= 0;
	m_ToolbarInformation[2].cyIntegral	= 0;
	m_ToolbarInformation[2].cxHeader	= 0;
	m_ToolbarInformation[2].lpText		= NULL;

	m_ToolbarInformation[3].wID			= ID_DRIVESTOOLBAR;
	m_ToolbarInformation[3].fMask		= RBBIM_ID|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE|RBBIM_IDEALSIZE|RBBIM_STYLE;
	m_ToolbarInformation[3].fStyle		= RBBS_BREAK|RBBS_USECHEVRON;
	m_ToolbarInformation[3].cx			= 0;
	m_ToolbarInformation[3].cxIdeal		= 0;
	m_ToolbarInformation[3].cxMinChild	= 0;
	m_ToolbarInformation[3].cyIntegral	= 0;
	m_ToolbarInformation[3].cxHeader	= 0;
	m_ToolbarInformation[3].lpText		= NULL;

	m_ToolbarInformation[4].wID			= ID_APPLICATIONSTOOLBAR;
	m_ToolbarInformation[4].fMask		= RBBIM_ID|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE|RBBIM_IDEALSIZE|RBBIM_STYLE;
	m_ToolbarInformation[4].fStyle		= RBBS_BREAK|RBBS_USECHEVRON;
	m_ToolbarInformation[4].cx			= 0;
	m_ToolbarInformation[4].cxIdeal		= 0;
	m_ToolbarInformation[4].cxMinChild	= 0;
	m_ToolbarInformation[4].cyIntegral	= 0;
	m_ToolbarInformation[4].cxHeader	= 0;
	m_ToolbarInformation[4].lpText		= NULL;
}

/*
 * Sets the default values used within the program.
 */
void Explorerplusplus::SetDefaultValues(void)
{
	/* User options. */
	m_bOpenNewTabNextToCurrent		= FALSE;
	m_bConfirmCloseTabs				= FALSE;
	m_bStickySelection				= TRUE;
	m_bShowFullTitlePath			= FALSE;
	m_bAlwaysOpenNewTab				= FALSE;
	m_bShowFolderSizes				= FALSE;
	m_bDisableFolderSizesNetworkRemovable	 = FALSE;
	m_bUnlockFolders				= TRUE;
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
	m_bHideRecycleBinGlobal			= FALSE;
	m_bHideSysVolInfoGlobal			= FALSE;
}

HMENU Explorerplusplus::CreateRebarHistoryMenu(BOOL bBack)
{
	HMENU hSubMenu = NULL;
	list<LPITEMIDLIST> lHistory;
	list<LPITEMIDLIST>::iterator itr;
	MENUITEMINFO mii;
	TCHAR szDisplayName[MAX_PATH];
	int iBase;
	int i = 0;

	if(bBack)
	{
		m_pActiveShellBrowser->GetBackHistory(&lHistory);

		iBase = ID_REBAR_MENU_BACK_START;
	}
	else
	{
		m_pActiveShellBrowser->GetForwardHistory(&lHistory);

		iBase = ID_REBAR_MENU_FORWARD_START;
	}

	if(lHistory.size() > 0)
	{
		hSubMenu = CreateMenu();

		for(itr = lHistory.begin();itr != lHistory.end();itr++)
		{
			GetDisplayName(*itr,szDisplayName,SHGDN_INFOLDER);

			mii.cbSize		= sizeof(mii);
			mii.fMask		= MIIM_ID|MIIM_STRING;
			mii.wID			= iBase + i + 1;
			mii.dwTypeData	= szDisplayName;
			InsertMenuItem(hSubMenu,i,TRUE,&mii);

			i++;

			CoTaskMemFree(*itr);
		}

		lHistory.clear();

		SetMenuOwnerDraw(hSubMenu);
	}

	return hSubMenu;
}

Explorerplusplus::CLoadSaveRegistry::CLoadSaveRegistry(Explorerplusplus *pContainer)
{
	m_pContainer = pContainer;
}

Explorerplusplus::CLoadSaveRegistry::~CLoadSaveRegistry()
{

}

/* IUnknown interface members. */
HRESULT __stdcall Explorerplusplus::CLoadSaveRegistry::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = NULL;

	if(*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall Explorerplusplus::CLoadSaveRegistry::AddRef(void)
{
	return ++m_iRefCount;
}

ULONG __stdcall Explorerplusplus::CLoadSaveRegistry::Release(void)
{
	m_iRefCount--;
	
	if(m_iRefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_iRefCount;
}

HRESULT Explorerplusplus::QueryService(REFGUID guidService,REFIID riid,void **ppv)
{
	*ppv = NULL;

	if(riid == IID_IShellView2)
	{
		*ppv = static_cast<IShellView2 *>(this);
	}
	else if(riid == IID_INewMenuClient)
	{
		*ppv = static_cast<INewMenuClient *>(this);
	}

	if(*ppv)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

HRESULT Explorerplusplus::CreateViewWindow2(LPSV2CVW2_PARAMS lpParams)
{
	return S_OK;
}

HRESULT Explorerplusplus::GetView(SHELLVIEWID *pvid,ULONG uView)
{
	return S_OK;
}

HRESULT Explorerplusplus::HandleRename(LPCITEMIDLIST pidlNew)
{
	return S_OK;
}

HRESULT Explorerplusplus::SelectAndPositionItem(LPCITEMIDLIST pidlItem,UINT uFlags,POINT *ppt)
{
	LPITEMIDLIST pidlComplete = NULL;
	LPITEMIDLIST pidlDirectory = NULL;

	/* The idlist passed is only a relative (child) one. Combine
	it with the tabs' current directory to get a full idlist. */
	pidlDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();
	pidlComplete = ILCombine(pidlDirectory,pidlItem);

	m_pActiveShellBrowser->QueueRename((LPITEMIDLIST)pidlComplete);

	CoTaskMemFree(pidlDirectory);
	CoTaskMemFree(pidlComplete);

	return S_OK;
}

HRESULT Explorerplusplus::GetWindow(HWND *)
{
	return S_OK;
}

HRESULT Explorerplusplus::ContextSensitiveHelp(BOOL bHelp)
{
	return S_OK;
}

HRESULT Explorerplusplus::TranslateAccelerator(MSG *msg)
{
	return S_OK;
}

HRESULT Explorerplusplus::EnableModeless(BOOL fEnable)
{
	return S_OK;
}

HRESULT Explorerplusplus::UIActivate(UINT uActivate)
{
	return S_OK;
}

HRESULT Explorerplusplus::Refresh(void)
{
	return S_OK;
}

HRESULT Explorerplusplus::CreateViewWindow(IShellView *psvPrevious,LPCFOLDERSETTINGS pfs,IShellBrowser *psb,RECT *prcView,HWND *phWnd)
{
	return S_OK;
}

HRESULT Explorerplusplus::DestroyViewWindow(void)
{
	return S_OK;
}

HRESULT Explorerplusplus::GetCurrentInfo(LPFOLDERSETTINGS pfs)
{
	return S_OK;
}

HRESULT Explorerplusplus::AddPropertySheetPages(DWORD dwReserved,LPFNSVADDPROPSHEETPAGE pfn,LPARAM lparam)
{
	return S_OK;
}

HRESULT Explorerplusplus::SaveViewState(void)
{
	return S_OK;
}

HRESULT Explorerplusplus::SelectItem(LPCITEMIDLIST pidlItem,SVSIF uFlags)
{
	return S_OK;
}

HRESULT Explorerplusplus::GetItemObject(UINT uItem,REFIID riid,void **ppv)
{
	return S_OK;
}

HRESULT Explorerplusplus::IncludeItems(NMCII_FLAGS *pFlags)
{
	/* pFlags will be one of:
	NMCII_ITEMS
	NMCII_FOLDERS
	Which one of these is selected determines which
	items are shown on the 'new' menu.
	If NMCII_ITEMS is selected, only files will be
	shown (meaning 'New Folder' will NOT appear).
	If NMCII_FOLDERS is selected, only folders will
	be shown (this means that in most cases, only
	'New Folder' and 'New Briefcase' will be shown).
	Therefore, to get all the items, OR the two flags
	together to show files and folders. */

	*pFlags = NMCII_ITEMS|NMCII_FOLDERS;

	return S_OK;
}

HRESULT Explorerplusplus::SelectAndEditItem(PCIDLIST_ABSOLUTE pidlItem,NMCSAEI_FLAGS flags)
{
	switch(flags)
	{
		/* This would usually cause the
		item to be selected first, then
		renamed. In this case however,
		the item is selected and renamed
		in one operation, so this state
		can be ignored. */
		case NMCSAEI_SELECT:
			break;

		/* Now, start an in-place rename
		of the item. */
		case NMCSAEI_EDIT:
			m_pActiveShellBrowser->QueueRename((LPITEMIDLIST)pidlItem);
			break;
	}

	return S_OK;
}