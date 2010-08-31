/******************************************************************
 *
 * Project: Explorer++
 * File: Explorer++.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Explorer++ is an Explorer-like file manager for
 * Windows XP, Vista and 7.
 *
 * This is the main module for Explorer++. Handles all
 * incoming GUI requests for the main window.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"


LRESULT CALLBACK	WndProcStub(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);
BOOL				ProcessCommandLine(TCHAR *pCommandLine);
void				SetLanguageModule(void);
void				ShowUsage(void);
void				ClearRegistrySettings(void);

int	g_nCmdShow = SW_SHOW;

HINSTANCE				g_hInstance;
HINSTANCE				g_hLanguageModule;
TCHAR					g_szLang[32];
BOOL					g_bForceLanguageLoad = FALSE;
list<TabDirectory_t>	g_TabDirs;

/* Search. */
HWND	g_hwndSearch;

/* Options dialog. */
HWND	g_hwndOptions;

CRITICAL_SECTION	g_csDirMonCallback;

DWORD ControlClasses		=	ICC_BAR_CLASSES | ICC_COOL_CLASSES |
								ICC_LISTVIEW_CLASSES | ICC_USEREX_CLASSES |
								ICC_STANDARD_CLASSES | ICC_LINK_CLASS;

/* TODO: Remove once custom menu image lists
have been corrected. */
extern HIMAGELIST himlMenu;

/* IUnknown interface members. */
HRESULT __stdcall CContainer::QueryInterface(REFIID iid, void **ppvObject)
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

ULONG __stdcall CContainer::AddRef(void)
{
	return ++m_iRefCount;
}

ULONG __stdcall CContainer::Release(void)
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
 * Constructor for the main CContainer class.
 */
CContainer::CContainer(HWND hwnd)
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
	m_bSelectingTreeViewDirectory	= FALSE;
	m_bCuttingItems					= FALSE;
	m_bTreeViewRightClick			= FALSE;
	m_bTabBeenDragged				= FALSE;
	m_bAlteredStatusBarParts		= FALSE;
	m_bTreeViewDelayEnabled			= FALSE;
	m_bSavePreferencesToXMLFile		= FALSE;
	m_bAttemptToolbarRestore		= FALSE;
	m_bCanUndo						= FALSE;
	m_bLanguageLoaded				= FALSE;
	m_bTCMouseCaptured				= FALSE;
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
	m_bWildcardDlgStateSaved		= FALSE;
	m_bSelectColumnsDlgStateSaved	= FALSE;
	m_bSetDefaultColumnsDlgStateSaved	= FALSE;
	m_bAddBookmarkDlgStateSaved		= FALSE;
	m_bDestroyFilesDlgStateSaved	= FALSE;
	m_bDisplayColorsDlgStateSaved	= FALSE;
	m_bFilterDlgStateSaved			= FALSE;
	m_bMassRenameDlgStateSaved		= FALSE;
	m_bMergeFilesDlgStateSaved		= FALSE;
	m_bOrganizeBookmarksDlgStateSaved	= FALSE;
	m_bDrivePropertiesDlgStateSaved	= FALSE;
	m_bSetFileAttributesDlgStateSaved	= FALSE;
	m_bSplitFileDlgStateSaved		= FALSE;
	m_bCustomizeColorsDlgStateSaved	= FALSE;
	m_bSearchDlgStateSaved			= FALSE;

	/* Search dialog. */
	m_bSearchSubFolders				= TRUE;

	m_rgbCompressed					= RGB(0,0,255);
	m_rgbEncrypted					= RGB(0,128,0);

	m_pTaskbarList3					= NULL;

	m_bBlockNext = FALSE;

	InitializeColorRules();

	int i = 0;

	for(i = 0;i < SIZEOF_ARRAY(m_ccCustomColors);i++)
	{
		m_ccCustomColors[i] = RGB(255,255,255);
	}

	m_crInitialColor = DEFAULT_INITIAL_COLOR;

	StringCchCopy(m_SearchPatternText,SIZEOF_ARRAY(m_SearchPatternText),
		EMPTY_STRING);
	StringCchCopy(m_szwsiText,SIZEOF_ARRAY(m_szwsiText),
		EMPTY_STRING);

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
 * Deconstructor for the main CContainer class.
 */
CContainer::~CContainer()
{
	m_pDirMon->Release();

	if(m_hDwmapi != NULL)
	{
		FreeLibrary(m_hDwmapi);
	}
}

void CContainer::InitializeMainToolbars(void)
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
void CContainer::SetDefaultValues(void)
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
	m_ReplaceExplorerMode			= REPLACEEXPLORER_NONE;
	m_bOneClickActivate				= FALSE;
	m_bAllowMultipleInstances		= TRUE;
	m_bForceSameTabWidth			= FALSE;
	m_bDoubleClickTabClose			= TRUE;
	m_bHandleZipFiles				= FALSE;
	m_bInsertSorted					= TRUE;
	m_bOverwriteExistingFilesConfirmation	= TRUE;
	m_bCheckBoxSelection			= FALSE;
	m_bForceSize					= FALSE;
	m_SizeDisplayFormat				= FORMAT_BYTES;
	m_bSynchronizeTreeview			= TRUE;
	m_bTVAutoExpandSelected			= FALSE;
	m_bCloseMainWindowOnTabClose	= TRUE;
	m_bLargeToolbarIcons			= FALSE;
	m_bShowTaskbarThumbnails		= TRUE;

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

/*
 * Registers the main window class.
 */
ATOM RegisterMainWindowClass(void)
{
	WNDCLASSEX wcex;

	/* Prepare the structure needed to create the main window. */
	wcex.cbSize			= sizeof(wcex);
	wcex.style			= 0;
	wcex.lpfnWndProc	= WndProcStub;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= sizeof(CContainer *);
	wcex.hInstance		= g_hInstance;
	wcex.hIcon			= (HICON)LoadImage(g_hInstance,MAKEINTRESOURCE(IDI_MAIN),IMAGE_ICON,48,48,LR_VGACOLOR);
	wcex.hIconSm		= (HICON)LoadImage(g_hInstance,MAKEINTRESOURCE(IDI_MAIN_SMALL),IMAGE_ICON,16,16,LR_VGACOLOR);
	wcex.hCursor		= LoadCursor(NULL,IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)NULL;
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= CLASS_NAME;

	return RegisterClassEx(&wcex);
}

typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcee,DWORD ProcessId,HANDLE hFile,
MINIDUMP_TYPE DumpType,PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

LONG WINAPI MyUnhandledExceptionFilter(struct _EXCEPTION_POINTERS *pExceptionInfo)
{
	HMODULE hDbgHelp;
	MINIDUMPWRITEDUMP pMiniDumpWriteDump;
	MINIDUMP_EXCEPTION_INFORMATION mei;
	HANDLE hFile;
	SYSTEMTIME stLocalTime;
	TCHAR szFileName[MAX_PATH];
	TCHAR szPath[MAX_PATH];
	TCHAR szAppName[] = _T("Explorer++");
	LONG ret = EXCEPTION_CONTINUE_SEARCH;

	hDbgHelp = LoadLibrary(_T("Dbghelp.dll"));

	if(hDbgHelp != NULL)
	{
		pMiniDumpWriteDump = (MINIDUMPWRITEDUMP)GetProcAddress(hDbgHelp,"MiniDumpWriteDump");

		if(pMiniDumpWriteDump != NULL)
		{
			GetLocalTime(&stLocalTime);

			MyExpandEnvironmentStrings(_T("%TEMP%"),szPath,SIZEOF_ARRAY(szPath));

			StringCchPrintf(szFileName,SIZEOF_ARRAY(szFileName),
				_T("%s\\%s%s-%02d%02d%04d-%02d%02d%02d.dmp"),
				szPath,szAppName,VERSION_NUMBER,stLocalTime.wDay,stLocalTime.wMonth,
				stLocalTime.wYear,stLocalTime.wHour,stLocalTime.wMinute,stLocalTime.wSecond);
			hFile = CreateFile(szFileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,NULL);

			if(hFile != INVALID_HANDLE_VALUE)
			{
				mei.ThreadId			= GetCurrentThreadId();
				mei.ExceptionPointers	= pExceptionInfo;
				mei.ClientPointers		= NULL;

				pMiniDumpWriteDump(GetCurrentProcess(),GetCurrentProcessId(),hFile,
					MiniDumpNormal,&mei,NULL,NULL);

				/* If this is enabled, it needs to have a proper error message, and block
				access to the main window. */
				/*StringCchPrintf(szMsg,SIZEOF_ARRAY(szMsg),_T("Explorer++ has encountered an error and needs to close. \
A minidump has been saved to:\n%s\nPlease report this to the developer."),szFileName);
				MessageBox(NULL,szMsg,szAppName,MB_OK);

				ret = EXCEPTION_EXECUTE_HANDLER;*/
			}
		}
	}

	return ret;
}

/*
* Program entry-point.
*/
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,
LPSTR lpCmdLine,int nCmdShow)
{
	HMODULE			hRichEditLib;
	HWND			hwnd;
	HACCEL			hAccl;
	OSVERSIONINFO	VersionInfo;
	HANDLE			hMutex = NULL;
	TCHAR			*pCommandLine	= NULL;
	MSG				msg;
	LONG			res;
	BOOL			bExit = FALSE;

	g_hInstance = hInstance;

	VersionInfo.dwOSVersionInfoSize	= sizeof(OSVERSIONINFO);

	if(GetVersionEx(&VersionInfo) != 0)
	{
		/* Are we running on at least Windows XP?
		If not, show an error message and exit. */
		if(VersionInfo.dwMajorVersion < WINDOWS_XP_MAJORVERSION)
		{
			MessageBox(NULL,
				_T("This application needs at least Windows XP or above to run properly."),
				_T("Explorer++"),MB_ICONERROR | MB_OK);

			return 0;
		}
	}

	/* Initialize OLE, as well as the various window classes that
	will be needed (listview, TreeView, comboboxex, etc.). */
	InitControlClasses(ControlClasses);
	OleInitialize(NULL);

	/* Process command line arguments. */
	pCommandLine = GetCommandLine();

	bExit = ProcessCommandLine(pCommandLine);

	/* Can't open folders that are children of the
	control panel. If the comand line only refers
	to folders that are children of the control panel,
	pass those folders to Windows Explorer, then exit. */
	if(g_TabDirs.size() > 0)
	{
		LPITEMIDLIST pidlControlPanel = NULL;
		LPITEMIDLIST pidl = NULL;

		HRESULT hr = SHGetFolderLocation(NULL,
			CSIDL_CONTROLS,NULL,0,&pidlControlPanel);

		if(SUCCEEDED(hr))
		{
			auto itr = g_TabDirs.begin();

			BOOL bControlPanelChild = FALSE;

			while(itr != g_TabDirs.end())
			{
				/* This could fail on a 64-bit version of
				Vista or Windows 7 if the executable is 32-bit,
				and the folder is 64-bit specific (as is the
				case with some of the folders under the control
				panel). */
				hr = GetIdlFromParsingName(itr->Dir,&pidl);

				bControlPanelChild = FALSE;

				if(SUCCEEDED(hr))
				{
					if(ILIsParent(pidlControlPanel,pidl,FALSE) &&
						!CompareIdls(pidlControlPanel,pidl))
					{
						bControlPanelChild = TRUE;
					}
					else
					{
						OSVERSIONINFO VersionInfo;

						VersionInfo.dwOSVersionInfoSize	= sizeof(OSVERSIONINFO);

						if(GetVersionEx(&VersionInfo) != 0)
						{
							if(VersionInfo.dwMajorVersion >= WINDOWS_VISTA_SEVEN_MAJORVERSION)
							{
								LPITEMIDLIST pidlControlPanelCategory = NULL;

								hr = GetIdlFromParsingName(CONTROL_PANEL_CATEGORY_VIEW,
									&pidlControlPanelCategory);

								if(SUCCEEDED(hr))
								{
									if(ILIsParent(pidlControlPanelCategory,pidl,FALSE) &&
										!CompareIdls(pidlControlPanelCategory,pidl))
									{
										bControlPanelChild = TRUE;
									}

									CoTaskMemFree(pidlControlPanelCategory);
								}
							}
						}
					}

					if(bControlPanelChild)
					{
						TCHAR szExplorerPath[MAX_PATH];

						MyExpandEnvironmentStrings(_T("%windir%\\explorer.exe"),
							szExplorerPath,SIZEOF_ARRAY(szExplorerPath));

						/* This is a child of the control panel,
						so send it to Windows Explorer to open
						directly. */
						ShellExecute(NULL,_T("open"),szExplorerPath,
							itr->Dir,NULL,SW_SHOWNORMAL);

						itr = g_TabDirs.erase(itr);
					}

					CoTaskMemFree(pidl);
				}


				if(!bControlPanelChild)
				{
					itr++;
				}
			}

			if(g_TabDirs.size() == 0)
			{
				bExit = TRUE;
			}

			CoTaskMemFree(pidlControlPanel);
		}
	}

	if(bExit)
	{
		return 0;
	}

	BOOL bAllowMultipleInstances = TRUE;
	BOOL bLoadSettingsFromXML;

	bLoadSettingsFromXML = TestConfigFileInternal();

	if(bLoadSettingsFromXML)
	{
		bAllowMultipleInstances = LoadAllowMultipleInstancesFromXML();
	}
	else
	{
		HKEY			hSettingsKey;
		LONG			ReturnValue;

		/* Open/Create the main key that is used to store data. */
		ReturnValue = RegOpenKeyEx(HKEY_CURRENT_USER,REG_SETTINGS_KEY,0,KEY_READ,&hSettingsKey);

		if(ReturnValue == ERROR_SUCCESS)
		{
			ReadDwordFromRegistry(hSettingsKey,_T("AllowMultipleInstances"),
				(LPDWORD)&bAllowMultipleInstances);

			RegCloseKey(hSettingsKey);
		}
	}

	/* Create the mutex regardless of the actual setting. For example,
	if the first instance is run, and multiple instances are allowed,
	and then disallowed, still need to be able to load back to the
	original instance. */
	hMutex = CreateMutex(NULL,TRUE,_T("Explorer++"));

	if(!bAllowMultipleInstances)
	{
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			HWND hPrev;

			hPrev = FindWindow(CLASS_NAME,NULL);

			if(hPrev != NULL)
			{
				list<TabDirectory_t>::iterator	itr;

				if(!g_TabDirs.empty())
				{
					for(itr = g_TabDirs.begin();itr != g_TabDirs.end();itr++)
					{
						COPYDATASTRUCT cds;

						cds.cbData	= (lstrlen(itr->Dir) + 1) * sizeof(TCHAR);
						cds.lpData	= itr->Dir;
						SendMessage(hPrev,WM_COPYDATA,NULL,(LPARAM)&cds);
					}
				}
				else
				{
					COPYDATASTRUCT cds;

					cds.cbData	= 0;
					cds.lpData	= NULL;
					SendMessage(hPrev,WM_COPYDATA,NULL,(LPARAM)&cds);
				}

				SetForegroundWindow(hPrev);
				ShowWindow(hPrev,SW_SHOW);
				CloseHandle(hMutex);
				return 0;
			}
		}
	}

	/* This dll is needed to create a richedit control. */
	hRichEditLib = LoadLibrary(_T("Riched20.dll"));

	res = RegisterMainWindowClass();

	if(res == 0)
	{
		MessageBox(NULL,_T("Could not register class"),WINDOW_NAME,MB_OK | MB_ICONERROR);

		FreeLibrary(hRichEditLib);
		OleUninitialize();

		return 0;
	}

	SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);

	g_nCmdShow = nCmdShow;

	/* Create the main window. This window will act as a
	container for all child windows created. */
	hwnd = CreateWindow(
	CLASS_NAME,
	WINDOW_NAME,
	WS_OVERLAPPEDWINDOW,
	CW_USEDEFAULT,
	CW_USEDEFAULT,
	CW_USEDEFAULT,
	CW_USEDEFAULT,
	NULL,
	NULL,
	hInstance,
	NULL);

	if(hwnd == NULL)
	{
		MessageBox(NULL,_T("Could not create main window."),WINDOW_NAME,MB_OK | MB_ICONERROR);

		FreeLibrary(hRichEditLib);
		OleUninitialize();

		return 0;
	}

	ShowWindow(hwnd,g_nCmdShow);
	UpdateWindow(hwnd);

	hAccl = LoadAccelerators(hInstance,MAKEINTRESOURCE(IDR_MAINACCELERATORS));

	/* Enter the message loop... */
	while(GetMessage(&msg,NULL,0,0) > 0)
	{
		/* TranslateMessage() must be in the inner loop,
		otherwise various accelerator keys (such as tab)
		would be taken even when the dialog has focus. */
		if(!IsDialogMessage(g_hwndSearch,&msg) &&
			!PropSheet_IsDialogMessage(g_hwndOptions,&msg))
		{
			if(!TranslateAccelerator(hwnd,hAccl,&msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		if(PropSheet_GetCurrentPageHwnd(g_hwndOptions) == NULL)
		{
			DestroyWindow(g_hwndOptions);
			g_hwndOptions = NULL;
		}
	}

	FreeLibrary(hRichEditLib);
	OleUninitialize();

	if(hMutex != NULL)
		CloseHandle(hMutex);

	return (int)msg.wParam;
}

/*
* Dummy window procedure. Simply transfers control
* to the 'real' window procedure.
*/
LRESULT CALLBACK WndProcStub(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	CContainer *pContainer = (CContainer *)GetWindowLongPtr(hwnd,GWLP_USERDATA);

	switch(Msg)
	{
		case WM_NCCREATE:
			/* Create a new CContainer object to assign to this window. */
			pContainer = new CContainer(hwnd);

			if(!pContainer)
			{
				PostQuitMessage(0);
				return 0;
			}

			/* Store the CContainer object pointer into the extra window bytes
			for this window. */
			SetWindowLongPtr(hwnd,GWLP_USERDATA,(LONG_PTR)pContainer);
			break;
	}

	/* Jump across to the member window function (will handle all requests). */
	if(pContainer != NULL)
		return pContainer->WindowProcedure(hwnd,Msg,wParam,lParam);
	else
		return DefWindowProc(hwnd,Msg,wParam,lParam);
}

/*
 * Main window procedure.
 */
LRESULT CALLBACK CContainer::WindowProcedure(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	if(Msg == m_uTaskbarButtonCreatedMessage)
	{
		HMODULE hUser32;
		ChangeWindowMessageFilterProc ChangeWindowMessageFilter;

		if((m_dwMajorVersion == WINDOWS_VISTA_SEVEN_MAJORVERSION &&
			m_dwMinorVersion == 0) ||
			m_dwMajorVersion < WINDOWS_VISTA_SEVEN_MAJORVERSION)
		{
			return 0;
		}

		if(!m_bShowTaskbarThumbnails)
		{
			return 0;
		}

		hUser32 = LoadLibrary(_T("user32.dll"));

		if(hUser32 != NULL)
		{
			/* If directly targeting Windows 7, this can be switched
			to static, rather than dynamic linking. */
			ChangeWindowMessageFilter = (ChangeWindowMessageFilterProc)GetProcAddress(hUser32,"ChangeWindowsMessageFilter");

			if(ChangeWindowMessageFilter != NULL)
			{
				ChangeWindowMessageFilter(WM_DWMSENDICONICTHUMBNAIL,MSGFLT_ADD);
				ChangeWindowMessageFilter(WM_DWMSENDICONICLIVEPREVIEWBITMAP,MSGFLT_ADD);
			}

			FreeLibrary(hUser32);
		}

		if(m_pTaskbarList3 != NULL)
		{
			m_pTaskbarList3->Release();
		}

		CoCreateInstance(CLSID_TaskbarList,NULL,CLSCTX_INPROC_SERVER,
			IID_ITaskbarList4,(LPVOID *)&m_pTaskbarList3);

		m_pTaskbarList3->HrInit();

		m_bTaskbarInitialised = TRUE;

		/* Add each of the jump list tasks. */
		SetupJumplistTasks();

		list<TabProxyInfo_t>::iterator itr;
		LPITEMIDLIST pidlDirectory = NULL;
		BOOL bActive;

		/* Register each of the tabs. */
		for(itr = m_TabProxyList.begin();itr != m_TabProxyList.end();itr++)
		{
			bActive = (itr->iTabId == m_iObjectIndex);

			RegisterTab(itr->hProxy,EMPTY_STRING,bActive);
			HandleTabText(itr->iTabId);
			SetTabIcon(itr->iTabId);

			CoTaskMemFree(pidlDirectory);
		}

		return 0;
	}
	else
	{
		switch(Msg)
		{
		case WM_CREATE:
			OnWindowCreate();
			break;

		case WM_SETFOCUS:
			OnSetFocus();
			return 0;
			break;

		case CBN_KEYDOWN:
			OnComboBoxKeyDown(wParam);
			break;

		case WM_INITMENU:
			m_pCustomMenu->OnInitMenu(wParam);
			SetProgramMenuItemStates((HMENU)wParam);
			break;

		case WM_MENUSELECT:
			StatusBarMenuSelect(wParam,lParam);
			break;

		case WM_MEASUREITEM:
			return OnMeasureItem(wParam,lParam);
			break;

		case WM_DRAWITEM:
			return OnDrawItem(wParam,lParam);
			break;

		case WM_DEVICECHANGE:
			OnDeviceChange(wParam,lParam);
			break;

		case WM_USER_UPDATEWINDOWS:
			UpdateWindowStates();
			break;

		case WM_USER_FILESADDED:
			/* Runs in the context of the main thread. Either
			occurs after the specified tab index has been
			freed (in which case nothing happens), or before. */
			if(CheckTabIdStatus((int)wParam))
				m_pShellBrowser[wParam]->DirectoryAltered();
			break;

		case WM_USER_RELEASEBROWSER:
			m_pShellBrowser[(int)wParam]->Release();
			m_pShellBrowser[(int)wParam] = NULL;
			m_pFolderView[(int)wParam]->Release();
			m_pFolderView[(int)wParam] = NULL;
			break;

		case WM_USER_TREEVIEW_GAINEDFOCUS:
			m_hLastActiveWindow = m_hTreeView;
			break;

		case WM_USER_TABMCLICK:
			OnTabMClick(wParam,lParam);
			break;

		case WM_USER_DISPLAYWINDOWRESIZED:
			OnDisplayWindowResized(wParam);
			break;

		case WM_USER_STARTEDBROWSING:
			OnStartedBrowsing((int)wParam,(TCHAR *)lParam);
			break;

		case WM_USER_NEWITEMINSERTED:
			OnShellNewItemCreated(lParam);
			break;

		case WM_USER_FOLDEREMPTY:
			{
				if((BOOL)lParam == TRUE)
					ListView_SetBackgroundImage(m_hListView[(int)wParam],IDB_FOLDEREMPTY);
				else
					ListView_SetBackgroundImage(m_hListView[(int)wParam],NULL);
			}
			break;

		case WM_USER_FILTERINGAPPLIED:
			{
				if((BOOL)lParam == TRUE)
					ListView_SetBackgroundImage(m_hListView[(int)wParam],IDB_FILTERINGAPPLIED);
				else
					ListView_SetBackgroundImage(m_hListView[(int)wParam],NULL);
			}
			break;

		case WM_USER_GETCOLUMNNAMEINDEX:
			return LookupColumnNameStringIndex((int)wParam);
			break;

		case WM_USER_DIRECTORYMODIFIED:
			OnDirectoryModified((int)wParam);
			break;

		case WM_USER_ASSOCCHANGED:
			OnAssocChanged();
			break;

		case WM_USER_HOLDERRESIZED:
			{
				RECT	rc;

				m_TreeViewWidth = (int)lParam + TREEVIEW_DRAG_OFFSET;

				GetClientRect(m_hContainer,&rc);

				SendMessage(m_hContainer,WM_SIZE,SIZE_RESTORED,(LPARAM)MAKELPARAM(rc.right,rc.bottom));
			}
			break;

		case WM_APP_FOLDERSIZECOMPLETED:
			{
				DWFolderSizeCompletion_t *pDWFolderSizeCompletion = NULL;
				TCHAR szFolderSize[32];
				TCHAR szSizeString[64];
				TCHAR szTotalSize[64];
				BOOL bValid = FALSE;

				pDWFolderSizeCompletion = (DWFolderSizeCompletion_t *)wParam;

				list<DWFolderSize_t>::iterator itr;

				/* First, make sure we should still display the
				results (we won't if the listview selection has
				changed, or this folder size was calculated for
				a tab other than the current one). */
				for(itr = m_DWFolderSizes.begin();itr != m_DWFolderSizes.end();itr++)
				{
					if(itr->uId == pDWFolderSizeCompletion->uId)
					{
						if(itr->iTabId == m_iObjectIndex)
						{
							bValid = itr->bValid;
						}

						m_DWFolderSizes.erase(itr);

						break;
					}
				}

				if(bValid)
				{
					FormatSizeString(pDWFolderSizeCompletion->liFolderSize,szFolderSize,
						SIZEOF_ARRAY(szFolderSize),m_bForceSize,m_SizeDisplayFormat);

					LoadString(g_hLanguageModule,IDS_GENERAL_TOTALSIZE,
						szTotalSize,SIZEOF_ARRAY(szTotalSize));

					StringCchPrintf(szSizeString,SIZEOF_ARRAY(szSizeString),
						_T("%s: %s"),szTotalSize,szFolderSize);

					/* TODO: The line index should be stored in some other (variable) way. */
					DisplayWindow_SetLine(m_hDisplayWindow,FOLDER_SIZE_LINE_INDEX,szSizeString);
				}

				free(pDWFolderSizeCompletion);
			}
			break;

		case WM_COPYDATA:
			{
				COPYDATASTRUCT *pcds = NULL;
				HRESULT hr;

				pcds = (COPYDATASTRUCT *)lParam;

				if(pcds->lpData != NULL)
				{
					BrowseFolder((TCHAR *)pcds->lpData,SBSP_ABSOLUTE,TRUE,TRUE,FALSE);
				}
				else
				{
					hr = BrowseFolder(m_DefaultTabDirectory,SBSP_ABSOLUTE,TRUE,TRUE,FALSE);

					if(FAILED(hr))
						BrowseFolder(m_DefaultTabDirectoryStatic,SBSP_ABSOLUTE,TRUE,TRUE,FALSE);
				}
			}
			break;

		case WM_NDW_RCLICK:
			OnNdwRClick(wParam,lParam);
			break;

		case WM_NDW_ICONRCLICK:
			OnNdwIconRClick(wParam,lParam);
			break;

		case WM_CHANGECBCHAIN:
			OnChangeCBChain(wParam,lParam);
			break;

		case WM_DRAWCLIPBOARD:
			OnDrawClipboard();
			break;

		case WM_APPCOMMAND:
			OnAppCommand(wParam,lParam);
			break;

		case WM_MENUCOMMAND:
			OnMenuCommand(wParam,lParam);
			break;

		case WM_COMMAND:
			return CommandHandler(hwnd,Msg,wParam,lParam);
			break;

		case WM_NOTIFY:
			return NotifyHandler(hwnd,Msg,wParam,lParam);
			break;

		case WM_SIZE:
			return OnSize(LOWORD(lParam),HIWORD(lParam));
			break;

		case WM_CLOSE:
			return OnClose();
			break;

		case WM_DESTROY:
			return OnDestroy();
			break;
		}
	}

	return DefWindowProc(hwnd,Msg,wParam,lParam);
}

void CContainer::OnMenuCommand(WPARAM wParam,LPARAM lParam)
{
	UINT	uMenuID;

	uMenuID = GetMenuItemID((HMENU)lParam,(int)wParam);

	if(uMenuID >= MENU_BOOKMARK_STARTID &&
	uMenuID <= MENU_BOOKMARK_ENDID)
	{
		TCHAR szDirectory[MAX_PATH];

		GetBookmarkMenuItemDirectory((HMENU)lParam,uMenuID,
			szDirectory,SIZEOF_ARRAY(szDirectory));

		BrowseFolder(szDirectory,SBSP_ABSOLUTE);
	}
}

/*
 * WM_COMMAND handler for main window.
 */
LRESULT CALLBACK CContainer::CommandHandler(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	/* Has a bookmark menu item been clicked? */
	if(!HIWORD(wParam) && LOWORD(wParam) >= MENU_BOOKMARK_STARTID &&
	LOWORD(wParam) <= MENU_BOOKMARK_ENDID)
	{
		TCHAR	szDirectory[MAX_PATH];

		GetBookmarkMenuItemDirectory(m_hBookmarksMenu,LOWORD(wParam),
			szDirectory,SIZEOF_ARRAY(szDirectory));

		ExpandAndBrowsePath(szDirectory);
	}
	else if(!HIWORD(wParam) && LOWORD(wParam) >= MENU_HEADER_STARTID &&
	LOWORD(wParam) <= MENU_HEADER_ENDID)
	{
		int iOffset;

		iOffset = LOWORD(wParam) - MENU_HEADER_STARTID;

		list<Column_t>				m_pActiveColumnList;
		list<Column_t>::iterator	itr;
		int							iItem = 0;
		unsigned int				*pHeaderList = NULL;

		m_pActiveShellBrowser->ExportCurrentColumns(&m_pActiveColumnList);

		GetColumnHeaderMenuList(&pHeaderList);

		/* Loop through all current items to find the item that was clicked, and
		flip its active state. */
		for(itr = m_pActiveColumnList.begin();itr != m_pActiveColumnList.end();itr++)
		{
			if(itr->id == pHeaderList[iOffset])
			{
				itr->bChecked = !itr->bChecked;
				break;
			}

			iItem++;
		}

		/* If it was the first column that was changed, need to refresh
		all columns. */
		if(iOffset == 0)
		{
			m_pActiveShellBrowser->ImportColumns(&m_pActiveColumnList,TRUE);

			RefreshTab(m_iObjectIndex);
		}
		else
		{
			m_pActiveShellBrowser->ImportColumns(&m_pActiveColumnList,FALSE);
		}
	}

	/* Was one of the items on the bookmarks toolbar clicked? */
	if(LOWORD(wParam) >= TOOLBAR_BOOKMARK_START &&
	LOWORD(wParam) <= (TOOLBAR_BOOKMARK_START +
	(MENU_BOOKMARK_ENDID - MENU_BOOKMARK_STARTID - 1)))
	{
		Bookmark_t	Bookmark;
		TBBUTTON	tbButton;
		int			iIndex;

		iIndex = (int)SendMessage(m_hBookmarksToolbar,TB_COMMANDTOINDEX,LOWORD(wParam),0);

		if(iIndex != -1)
		{
			SendMessage(m_hBookmarksToolbar,TB_GETBUTTON,iIndex,(LPARAM)&tbButton);

			m_Bookmark.RetrieveBookmark((void *)tbButton.dwData,&Bookmark);

			/* If the toolbar item is a bookmark, simply navigate
			to its directory. If it's a folder, open a menu with
			its sub-items on. */
			if(Bookmark.Type == BOOKMARK_TYPE_BOOKMARK)
			{
				ExpandAndBrowsePath(Bookmark.szLocation);
			}
			else
			{
				HMENU		hMenu;
				Bookmark_t	ChildBookmark;
				RECT		rc;
				HRESULT		hr;

				hMenu = CreatePopupMenu();

				MENUINFO mi;
				mi.cbSize	= sizeof(MENUINFO);
				mi.fMask	= MIM_STYLE;
				mi.dwStyle	= MNS_NOTIFYBYPOS;
				SetMenuInfo(hMenu,&mi);

				hr = m_Bookmark.GetChild(&Bookmark,&ChildBookmark);

				if(SUCCEEDED(hr))
				{
					InsertBookmarksIntoMenuInternal(hMenu,&ChildBookmark,0,MENU_BOOKMARK_STARTID);
				}
				else
				{
					InsertBookmarksIntoMenuInternal(hMenu,NULL,0,MENU_BOOKMARK_STARTID);
				}

				SendMessage(m_hBookmarksToolbar,TB_GETITEMRECT,iIndex,(LPARAM)&rc);
				MapWindowPoints(m_hBookmarksToolbar,NULL,(LPPOINT)&rc,2);

				TrackPopupMenu(hMenu,TPM_LEFTALIGN,rc.left,rc.bottom,0,hwnd,NULL);
			}
		}
	}

	if(LOWORD(wParam) >= TOOLBAR_APPLICATIONS_ID_START)
	{
		int iIndex;

		iIndex = (int)SendMessage(m_hApplicationToolbar,
			TB_COMMANDTOINDEX,LOWORD(wParam),0);

		ApplicationToolbarOpenItem(iIndex,EMPTY_STRING);
	}
	else if(LOWORD(wParam) >= TOOLBAR_DRIVES_ID_START)
	{
		/* Was one of the buttons on the drives toolbar clicked? */
		TBBUTTON	tbButton;
		TCHAR		*pszDrivePath = NULL;
		LRESULT		lResult;
		int			iIndex;

		iIndex = (int)SendMessage(m_hDrivesToolbar,TB_COMMANDTOINDEX,LOWORD(wParam),0);

		if(iIndex != -1)
		{
			lResult = SendMessage(m_hDrivesToolbar,TB_GETBUTTON,iIndex,(LPARAM)&tbButton);

			if(lResult)
			{
				pszDrivePath = (TCHAR *)tbButton.dwData;

				BrowseFolder(pszDrivePath,SBSP_ABSOLUTE);
			}
		}
	}

	switch(LOWORD(wParam))
	{
		case TOOLBAR_NEWTAB:
		case IDM_FILE_NEWTAB:
			OnNewTab();
			break;

		case TABTOOLBAR_CLOSE:
		case IDM_FILE_CLOSETAB:
			OnCloseTab();
			break;

		case IDM_FILE_CLONEWINDOW:
			OnCloneWindow();
			break;

		case IDM_FILE_SAVEDIRECTORYLISTING:
			OnSaveDirectoryListing();
			break;

		case TOOLBAR_SHOWCOMMANDPROMPT:
		case IDM_FILE_SHOWCOMMANDPROMPT:
			StartCommandPrompt(m_CurrentDirectory);
			break;

		case IDM_FILE_COPYFOLDERPATH:
			CopyTextToClipboard(m_CurrentDirectory);
			break;

		case IDM_FILE_COPYITEMPATH:
			OnCopyItemPath();
			break;

		case IDM_FILE_COPYUNIVERSALFILEPATHS:
			OnCopyUniversalPaths();
			break;

		case IDM_FILE_COPYCOLUMNTEXT:
			CopyColumnInfoToClipboard();
			break;

		case IDM_FILE_SETFILEATTRIBUTES:
			OnSetFileAttributes();
			break;

		case TOOLBAR_DELETE:
		case IDM_FILE_DELETE:
			OnFileDelete(FALSE);
			break;

		case IDM_FILE_DELETEPERMANENTLY:
			OnFileDelete(TRUE);
			break;

		case IDM_FILE_RENAME:
		case IDA_FILE_RENAME:
			OnFileRename();
			break;

		case TOOLBAR_PROPERTIES:
		case IDM_FILE_PROPERTIES:
		case IDM_RCLICK_PROPERTIES:
			OnShowFileProperties();
			break;

		case IDM_FILE_EXIT:
			SendMessage(hwnd,WM_CLOSE,0,0);
			break;

		case IDM_EDIT_UNDO:
			OnUndo();
			break;

		case TOOLBAR_COPY:
		case IDM_EDIT_COPY:
			OnCopy(TRUE);
			break;

		case TOOLBAR_CUT:
		case IDM_EDIT_CUT:
			OnCopy(FALSE);
			break;

		case TOOLBAR_PASTE:
		case IDM_EDIT_PASTE:
			OnPaste();
			break;

		case IDM_EDIT_PASTESHORTCUT:
			PasteLinksToClipboardFiles(m_CurrentDirectory);
			break;

		case IDM_EDIT_PASTEHARDLINK:
			PasteHardLinks(m_CurrentDirectory);
			break;

		case IDM_EDIT_COPYTOFOLDER:
		case TOOLBAR_COPYTO:
			CopyToFolder(FALSE);
			break;

		case TOOLBAR_MOVETO:
		case IDM_EDIT_MOVETOFOLDER:
			CopyToFolder(TRUE);
			break;

		case IDM_EDIT_SELECTALL:
			m_bCountingUp = TRUE;
			ListView_SelectAllItems(m_hActiveListView);
			SetFocus(m_hActiveListView);
			break;

		case IDM_EDIT_INVERTSELECTION:
			m_bInverted = TRUE;
			m_nSelectedOnInvert = m_nSelected;
			ListView_InvertSelection(m_hActiveListView);
			SetFocus(m_hActiveListView);
			break;

		case IDM_EDIT_SELECTALLFOLDERS:
			HighlightSimilarFiles(m_hActiveListView);
			SetFocus(m_hActiveListView);
			break;

		case IDM_EDIT_SELECTNONE:
			m_bCountingDown = TRUE;
			ListView_DeselectAllItems(m_hActiveListView);
			SetFocus(m_hActiveListView);
			break;

		case IDM_EDIT_WILDCARDSELECTION:
			OnWildcardSelect(TRUE);
			break;

		case IDM_EDIT_WILDCARDDESELECT:
			OnWildcardSelect(FALSE);
			break;

		case IDM_EDIT_SHOWFILESLACK:
			OnSaveFileSlack();
			break;

		case IDM_EDIT_RESOLVELINK:
			OnResolveLink();
			break;

		case IDM_VIEW_STATUSBAR:
			m_bShowStatusBar = !m_bShowStatusBar;
			lShowWindow(m_hStatusBar,m_bShowStatusBar);
			ResizeWindows();
			break;

		case IDM_VIEW_FOLDERS:
		case TOOLBAR_FOLDERS:
			ToggleFolders();
			break;

		case IDM_VIEW_DISPLAYWINDOW:
			m_bShowDisplayWindow = !m_bShowDisplayWindow;
			lShowWindow(m_hDisplayWindow,m_bShowDisplayWindow);
			ResizeWindows();
			break;

		case IDM_TOOLBARS_ADDRESSBAR:
			m_bShowAddressBar = !m_bShowAddressBar;
			ShowMainRebarBand(m_hAddressToolbar,m_bShowAddressBar);
			AdjustFolderPanePosition();
			ResizeWindows();
			break;

		case IDM_TOOLBARS_MAINTOOLBAR:
			m_bShowMainToolbar = !m_bShowMainToolbar;
			ShowMainRebarBand(m_hMainToolbar,m_bShowMainToolbar);
			AdjustFolderPanePosition();
			ResizeWindows();
			break;

		case IDM_TOOLBARS_BOOKMARKSTOOLBAR:
			m_bShowBookmarksToolbar = !m_bShowBookmarksToolbar;
			ShowMainRebarBand(m_hBookmarksToolbar,m_bShowBookmarksToolbar);
			AdjustFolderPanePosition();
			ResizeWindows();
			break;

		case IDM_TOOLBARS_DRIVES:
			m_bShowDrivesToolbar = !m_bShowDrivesToolbar;
			ShowMainRebarBand(m_hDrivesToolbar,m_bShowDrivesToolbar);
			AdjustFolderPanePosition();
			ResizeWindows();
			break;

		case IDM_TOOLBARS_APPLICATIONTOOLBAR:
			m_bShowApplicationToolbar = !m_bShowApplicationToolbar;
			ShowMainRebarBand(m_hApplicationToolbar,m_bShowApplicationToolbar);
			AdjustFolderPanePosition();
			ResizeWindows();
			break;

		case IDM_TOOLBARS_LOCKTOOLBARS:
			OnLockToolbars();
			break;

		case IDM_TOOLBARS_CUSTOMIZE:
			SendMessage(m_hMainToolbar,TB_CUSTOMIZE,0,0);
			break;

		case IDM_VIEW_EXTRALARGEICONS:
			m_pFolderView[m_iObjectIndex]->SetCurrentViewMode(VM_EXTRALARGEICONS);
			break;

		case IDM_VIEW_LARGEICONS:
			m_pFolderView[m_iObjectIndex]->SetCurrentViewMode(VM_LARGEICONS);
			break;

		case IDM_VIEW_ICONS:
			m_pFolderView[m_iObjectIndex]->SetCurrentViewMode(VM_ICONS);
			break;

		case IDM_VIEW_SMALLICONS:
			m_pFolderView[m_iObjectIndex]->SetCurrentViewMode(VM_SMALLICONS);
			break;

		case IDM_VIEW_LIST:
			m_pFolderView[m_iObjectIndex]->SetCurrentViewMode(VM_LIST);
			break;

		case IDM_VIEW_DETAILS:
			m_pFolderView[m_iObjectIndex]->SetCurrentViewMode(VM_DETAILS);
			break;

		case IDM_VIEW_THUMBNAILS:
			m_pFolderView[m_iObjectIndex]->SetCurrentViewMode(VM_THUMBNAILS);
			break;

		case IDM_VIEW_TILES:
			m_pFolderView[m_iObjectIndex]->SetCurrentViewMode(VM_TILES);
			break;

		case IDM_VIEW_CHANGEDISPLAYCOLOURS:
			DialogBoxParam(g_hLanguageModule,MAKEINTRESOURCE(IDD_DISPLAYCOLOURS),
				hwnd,ChangeDisplayColoursStub,(LPARAM)this);
			break;

		case IDM_FILTER_FILTERRESULTS:
			DialogBoxParam(g_hLanguageModule,MAKEINTRESOURCE(IDD_FILTER),
				hwnd,FilterProcStub,(LPARAM)this);
			break;

		case IDM_FILTER_APPLYFILTER:
			SetFilterStatus();
			break;

		case IDM_SORTBY_NAME:
			OnSortBy(FSM_NAME);
			break;

		case IDM_SORTBY_SIZE:
			OnSortBy(FSM_SIZE);
			break;

		case IDM_SORTBY_TYPE:
			OnSortBy(FSM_TYPE);
			break;

		case IDM_SORTBY_DATEMODIFIED:
			OnSortBy(FSM_DATEMODIFIED);
			break;

		case IDM_SORTBY_TOTALSIZE:
			OnSortBy(FSM_TOTALSIZE);
			break;

		case IDM_SORTBY_FREESPACE:
			OnSortBy(FSM_FREESPACE);
			break;

		case IDM_SORTBY_DATEDELETED:
			OnSortBy(FSM_DATEDELETED);
			break;

		case IDM_SORTBY_ORIGINALLOCATION:
			OnSortBy(FSM_ORIGINALLOCATION);
			break;

		case IDM_SORTBY_ATTRIBUTES:
			OnSortBy(FSM_ATTRIBUTES);
			break;

		case IDM_SORTBY_REALSIZE:
			OnSortBy(FSM_REALSIZE);
			break;

		case IDM_SORTBY_SHORTNAME:
			OnSortBy(FSM_SHORTNAME);
			break;

		case IDM_SORTBY_OWNER:
			OnSortBy(FSM_OWNER);
			break;

		case IDM_SORTBY_PRODUCTNAME:
			OnSortBy(FSM_PRODUCTNAME);
			break;

		case IDM_SORTBY_COMPANY:
			OnSortBy(FSM_COMPANY);
			break;

		case IDM_SORTBY_DESCRIPTION:
			OnSortBy(FSM_DESCRIPTION);
			break;

		case IDM_SORTBY_FILEVERSION:
			OnSortBy(FSM_FILEVERSION);
			break;

		case IDM_SORTBY_PRODUCTVERSION:
			OnSortBy(FSM_PRODUCTVERSION);
			break;

		case IDM_SORTBY_SHORTCUTTO:
			OnSortBy(FSM_SHORTCUTTO);
			break;

		case IDM_SORTBY_HARDLINKS:
			OnSortBy(FSM_HARDLINKS);
			break;

		case IDM_SORTBY_EXTENSION:
			OnSortBy(FSM_EXTENSION);
			break;

		case IDM_SORTBY_CREATED:
			OnSortBy(FSM_CREATED);
			break;

		case IDM_SORTBY_ACCESSED:
			OnSortBy(FSM_ACCESSED);
			break;

		case IDM_SORTBY_TITLE:
			OnSortBy(FSM_TITLE);
			break;

		case IDM_SORTBY_SUBJECT:
			OnSortBy(FSM_SUBJECT);
			break;

		case IDM_SORTBY_AUTHOR:
			OnSortBy(FSM_AUTHOR);
			break;

		case IDM_SORTBY_KEYWORDS:
			OnSortBy(FSM_KEYWORDS);
			break;

		case IDM_SORTBY_COMMENTS:
			OnSortBy(FSM_COMMENTS);
			break;

		case IDM_SORTBY_CAMERAMODEL:
			OnSortBy(FSM_CAMERAMODEL);
			break;

		case IDM_SORTBY_DATETAKEN:
			OnSortBy(FSM_DATETAKEN);
			break;

		case IDM_SORTBY_WIDTH:
			OnSortBy(FSM_WIDTH);
			break;

		case IDM_SORTBY_HEIGHT:
			OnSortBy(FSM_HEIGHT);
			break;

		case IDM_SORTBY_VIRTUALCOMMENTS:
			OnSortBy(FSM_VIRTUALCOMMENTS);
			break;

		case IDM_SORTBY_FILESYSTEM:
			OnSortBy(FSM_FILESYSTEM);
			break;

		case IDM_SORTBY_VIRTUALTYPE:
			OnSortBy(FSM_VIRTUALTYPE);
			break;

		case IDM_SORTBY_NUMPRINTERDOCUMENTS:
			OnSortBy(FSM_NUMPRINTERDOCUMENTS);
			break;

		case IDM_SORTBY_PRINTERSTATUS:
			OnSortBy(FSM_PRINTERSTATUS);
			break;

		case IDM_SORTBY_PRINTERCOMMENTS:
			OnSortBy(FSM_PRINTERCOMMENTS);
			break;

		case IDM_SORTBY_PRINTERLOCATION:
			OnSortBy(FSM_PRINTERLOCATION);
			break;

		case IDM_SORTBY_NETWORKADAPTER_STATUS:
			OnSortBy(FSM_NETWORKADAPTER_STATUS);
			break;

		case IDM_GROUPBY_NAME:
			OnGroupBy(FSM_NAME);
			break;

		case IDM_GROUPBY_SIZE:
			OnGroupBy(FSM_SIZE);
			break;

		case IDM_GROUPBY_TYPE:
			OnGroupBy(FSM_TYPE);
			break;

		case IDM_GROUPBY_DATEMODIFIED:
			OnGroupBy(FSM_DATEMODIFIED);
			break;

		case IDM_GROUPBY_TOTALSIZE:
			OnGroupBy(FSM_TOTALSIZE);
			break;

		case IDM_GROUPBY_FREESPACE:
			OnGroupBy(FSM_FREESPACE);
			break;

		case IDM_GROUPBY_DATEDELETED:
			OnGroupBy(FSM_DATEDELETED);
			break;

		case IDM_GROUPBY_ORIGINALLOCATION:
			OnGroupBy(FSM_ORIGINALLOCATION);
			break;

		case IDM_GROUPBY_ATTRIBUTES:
			OnGroupBy(FSM_ATTRIBUTES);
			break;

		case IDM_GROUPBY_REALSIZE:
			OnGroupBy(FSM_REALSIZE);
			break;

		case IDM_GROUPBY_SHORTNAME:
			OnGroupBy(FSM_SHORTNAME);
			break;

		case IDM_GROUPBY_OWNER:
			OnGroupBy(FSM_OWNER);
			break;

		case IDM_GROUPBY_PRODUCTNAME:
			OnGroupBy(FSM_PRODUCTNAME);
			break;

		case IDM_GROUPBY_COMPANY:
			OnGroupBy(FSM_COMPANY);
			break;

		case IDM_GROUPBY_DESCRIPTION:
			OnGroupBy(FSM_DESCRIPTION);
			break;

		case IDM_GROUPBY_FILEVERSION:
			OnGroupBy(FSM_FILEVERSION);
			break;

		case IDM_GROUPBY_PRODUCTVERSION:
			OnGroupBy(FSM_PRODUCTVERSION);
			break;

		case IDM_GROUPBY_SHORTCUTTO:
			OnGroupBy(FSM_SHORTCUTTO);
			break;

		case IDM_GROUPBY_HARDLINKS:
			OnGroupBy(FSM_HARDLINKS);
			break;

		case IDM_GROUPBY_EXTENSION:
			OnGroupBy(FSM_EXTENSION);
			break;

		case IDM_GROUPBY_CREATED:
			OnGroupBy(FSM_CREATED);
			break;

		case IDM_GROUPBY_ACCESSED:
			OnGroupBy(FSM_ACCESSED);
			break;

		case IDM_GROUPBY_TITLE:
			OnGroupBy(FSM_TITLE);
			break;

		case IDM_GROUPBY_SUBJECT:
			OnGroupBy(FSM_SUBJECT);
			break;

		case IDM_GROUPBY_AUTHOR:
			OnGroupBy(FSM_AUTHOR);
			break;

		case IDM_GROUPBY_KEYWORDS:
			OnGroupBy(FSM_KEYWORDS);
			break;

		case IDM_GROUPBY_COMMENTS:
			OnGroupBy(FSM_COMMENTS);
			break;

		case IDM_GROUPBY_CAMERAMODEL:
			OnGroupBy(FSM_CAMERAMODEL);
			break;

		case IDM_GROUPBY_DATETAKEN:
			OnGroupBy(FSM_DATETAKEN);
			break;

		case IDM_GROUPBY_WIDTH:
			OnGroupBy(FSM_WIDTH);
			break;

		case IDM_GROUPBY_HEIGHT:
			OnGroupBy(FSM_HEIGHT);
			break;

		case IDM_GROUPBY_VIRTUALCOMMENTS:
			OnGroupBy(FSM_VIRTUALCOMMENTS);
			break;

		case IDM_GROUPBY_FILESYSTEM:
			OnGroupBy(FSM_FILESYSTEM);
			break;

		case IDM_GROUPBY_VIRTUALTYPE:
			OnGroupBy(FSM_VIRTUALTYPE);
			break;

		case IDM_GROUPBY_NUMPRINTERDOCUMENTS:
			OnGroupBy(FSM_NUMPRINTERDOCUMENTS);
			break;

		case IDM_GROUPBY_PRINTERSTATUS:
			OnGroupBy(FSM_PRINTERSTATUS);
			break;

		case IDM_GROUPBY_PRINTERCOMMENTS:
			OnGroupBy(FSM_PRINTERCOMMENTS);
			break;

		case IDM_GROUPBY_PRINTERLOCATION:
			OnGroupBy(FSM_PRINTERLOCATION);
			break;

		case IDM_GROUPBY_NETWORKADAPTER_STATUS:
			OnGroupBy(FSM_NETWORKADAPTER_STATUS);
			break;

		case IDM_ARRANGEICONSBY_ASCENDING:
			OnSortByAscending(TRUE);
			break;

		case IDM_ARRANGEICONSBY_DESCENDING:
			OnSortByAscending(FALSE);
			break;
			
		case IDM_ARRANGEICONSBY_AUTOARRANGE:
			m_pActiveShellBrowser->ToggleAutoArrange();
			break;

		case IDM_ARRANGEICONSBY_SHOWINGROUPS:
			m_pActiveShellBrowser->ToggleGrouping();
			break;

		case IDM_VIEW_SHOWHIDDENFILES:
			ShowHiddenFiles();
			break;

		case TOOLBAR_REFRESH:
		case IDM_VIEW_REFRESH:
			OnRefresh();
			break;

		case IDM_SORTBY_MORE:
		case IDM_VIEW_SELECTCOLUMNS:
			DialogBoxParam(g_hLanguageModule,MAKEINTRESOURCE(IDD_SELECTCOLUMNS),
				hwnd,SelectColumnsProcStub,(LPARAM)this);
			UpdateArrangeMenuItems();
			break;

		case IDM_VIEW_AUTOSIZECOLUMNS:
			OnAutoSizeColumns();
			break;

		case IDM_VIEW_SAVECOLUMNLAYOUTASDEFAULT:
			{
				/* Dump the columns from the current tab, and save
				them as the default columns for the appropriate folder
				type.. */
				IShellFolder *pShellFolder = NULL;
				list<Column_t> pActiveColumnList;
				LPITEMIDLIST pidl = NULL;
				LPITEMIDLIST pidlDrives = NULL;
				LPITEMIDLIST pidlControls = NULL;
				LPITEMIDLIST pidlBitBucket = NULL;
				LPITEMIDLIST pidlPrinters = NULL;
				LPITEMIDLIST pidlConnections = NULL;
				LPITEMIDLIST pidlNetwork = NULL;

				m_pActiveShellBrowser->ExportCurrentColumns(&pActiveColumnList);

				pidl = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

				SHGetFolderLocation(NULL,CSIDL_DRIVES,NULL,0,&pidlDrives);
				SHGetFolderLocation(NULL,CSIDL_CONTROLS,NULL,0,&pidlControls);
				SHGetFolderLocation(NULL,CSIDL_BITBUCKET,NULL,0,&pidlBitBucket);
				SHGetFolderLocation(NULL,CSIDL_PRINTERS,NULL,0,&pidlPrinters);
				SHGetFolderLocation(NULL,CSIDL_CONNECTIONS,NULL,0,&pidlConnections);
				SHGetFolderLocation(NULL,CSIDL_NETWORK,NULL,0,&pidlNetwork);

				SHGetDesktopFolder(&pShellFolder);

				if(pShellFolder->CompareIDs(SHCIDS_CANONICALONLY,pidl,pidlDrives) == 0)
				{
					m_MyComputerColumnList = pActiveColumnList;
				}
				else if(pShellFolder->CompareIDs(SHCIDS_CANONICALONLY,pidl,pidlControls) == 0)
				{
					m_ControlPanelColumnList = pActiveColumnList;
				}
				else if(pShellFolder->CompareIDs(SHCIDS_CANONICALONLY,pidl,pidlBitBucket) == 0)
				{
					m_RecycleBinColumnList = pActiveColumnList;
				}
				else if(pShellFolder->CompareIDs(SHCIDS_CANONICALONLY,pidl,pidlPrinters) == 0)
				{
					m_PrintersColumnList = pActiveColumnList;
				}
				else if(pShellFolder->CompareIDs(SHCIDS_CANONICALONLY,pidl,pidlConnections) == 0)
				{
					m_NetworkConnectionsColumnList = pActiveColumnList;
				}
				else if(pShellFolder->CompareIDs(SHCIDS_CANONICALONLY,pidl,pidlNetwork) == 0)
				{
					m_MyNetworkPlacesColumnList = pActiveColumnList;
				}
				else
				{
					m_RealFolderColumnList = pActiveColumnList;
				}

				pActiveColumnList.clear();

				pShellFolder->Release();

				CoTaskMemFree(pidl);
			}
			break;

		case TOOLBAR_NEWFOLDER:
		case IDM_ACTIONS_NEWFOLDER:
			OnCreateNewFolder();
			break;

		case IDM_ACTIONS_MERGEFILES:
			DialogBoxParam(g_hLanguageModule,MAKEINTRESOURCE(IDD_MERGEFILES),
			hwnd,MergeFilesProcStub,(LPARAM)this);
			break;

		case IDM_ACTIONS_SPLITFILE:
			DialogBoxParam(g_hLanguageModule,MAKEINTRESOURCE(IDD_SPLITFILE),
			hwnd,SplitFileProcStub,(LPARAM)this);
			break;

		case IDM_ACTIONS_DESTROYFILES:
			DialogBoxParam(g_hLanguageModule,MAKEINTRESOURCE(IDD_DESTROYFILES),
			hwnd,DestroyFilesProcStub,(LPARAM)this);
			break;

		case IDM_GO_BACK:
		case TOOLBAR_BACK:
			OnBrowseBack();
			break;

		case IDM_GO_FORWARD:
		case TOOLBAR_FORWARD:
			OnBrowseForward();
			break;

		case IDM_GO_UPONELEVEL:
		case TOOLBAR_UP:
			OnNavigateUp();
			break;

		case IDM_GO_MYCOMPUTER:
			GotoFolder(CSIDL_DRIVES);
			break;

		case IDM_GO_MYDOCUMENTS:
			GotoFolder(CSIDL_PERSONAL);
			break;

		case IDM_GO_MYMUSIC:
			GotoFolder(CSIDL_MYMUSIC);
			break;

		case IDM_GO_MYPICTURES:
			GotoFolder(CSIDL_MYPICTURES);
			break;

		case IDM_GO_DESKTOP:
			GotoFolder(CSIDL_DESKTOP);
			break;

		case IDM_GO_RECYCLEBIN:
			GotoFolder(CSIDL_BITBUCKET);
			break;

		case IDM_GO_CONTROLPANEL:
			GotoFolder(CSIDL_CONTROLS);
			break;

		case IDM_GO_PRINTERS:
			GotoFolder(CSIDL_PRINTERS);
			break;

		case IDM_GO_CDBURNING:
			GotoFolder(CSIDL_CDBURN_AREA);
			break;

		case IDM_GO_MYNETWORKPLACES:
			GotoFolder(CSIDL_NETWORK);
			break;

		case IDM_GO_NETWORKCONNECTIONS:
			GotoFolder(CSIDL_CONNECTIONS);
			break;

		case TOOLBAR_ADDBOOKMARK:
		case IDM_BOOKMARKS_BOOKMARKTHISTAB:
			{
				AddBookmarkInfo_t abi;

				abi.pContainer		= (void *)this;
				abi.pParentBookmark	= NULL;
				abi.pidlDirectory	= m_pActiveShellBrowser->QueryCurrentDirectoryIdl();
				abi.bExpandInitial	= FALSE;

				DialogBoxParam(g_hLanguageModule,MAKEINTRESOURCE(IDD_ADD_BOOKMARK),
					hwnd,BookmarkTabDlgProcStub,(LPARAM)&abi);

				CoTaskMemFree(abi.pidlDirectory);

				/* It's possible the dialog may have been cancelled
				or closed, but that the bookmarks menu still needs
				to be updated. This is the case when (for example),
				a new bookmark folder is created, and the dialog
				is cancelled. */
				InsertBookmarksIntoMenu();
			}
			break;

		case TOOLBAR_ORGANIZEBOOKMARKS:
		case IDM_BOOKMARKS_ORGANIZEBOOKMARKS:
			DialogBoxParam(g_hLanguageModule,MAKEINTRESOURCE(IDD_ORGANIZE_BOOKMARKS),
				m_hContainer,OrganizeBookmarksStub,(LPARAM)this);
			break;

		case TOOLBAR_SEARCH:
		case IDM_TOOLS_SEARCH:
			if(g_hwndSearch == NULL)
			{
				g_hwndSearch = CreateDialogParam(g_hLanguageModule,MAKEINTRESOURCE(IDD_SEARCH),
					hwnd,SearchProcStub,(LPARAM)this);
			}
			else
			{
				SetFocus(g_hwndSearch);
			}
			break;

		case IDM_TOOLS_CUSTOMIZECOLORS:
			DialogBoxParam(g_hLanguageModule,MAKEINTRESOURCE(IDD_CUSTOMIZECOLORS),
				hwnd,ColorFilteringProcStub,(LPARAM)this);
			break;

		case IDM_TOOLS_OPTIONS:
			if(g_hwndOptions == NULL)
			{
				OnShowOptions();
			}
			else
			{
				SetFocus(g_hwndOptions);
			}
			break;

		case IDM_HELP_ABOUT:
			DialogBox(g_hLanguageModule,MAKEINTRESOURCE(IDD_ABOUT),hwnd,AboutDialogProcedure);
			break;

		case IDM_HELP_HELP:
			break;


		case IDA_NEXTTAB:
			SelectAdjacentTab(TRUE);
			break;

		case IDA_PREVIOUSTAB:
			SelectAdjacentTab(FALSE);
			break;

		case IDA_ADDRESSBAR:
			SetFocus(m_hAddressBar);
			break;

		case IDA_COMBODROPDOWN:
			SetFocus(m_hAddressBar);
			SendMessage(m_hAddressBar,CB_SHOWDROPDOWN,TRUE,0);
			break;

		case IDA_PREVIOUSWINDOW:
			OnPreviousWindow();
			break;

		case IDA_NEXTWINDOW:
			OnNextWindow();
			break;

		case IDA_RCLICK:
			OnIdaRClick();
			break;

		case IDA_TAB_DUPLICATETAB:
			OnDuplicateTab(m_iTabSelectedItem);
			break;

		case IDA_HOME:
			OnHome();
			break;

		case IDA_TAB1:
			OnSelectTab(0);
			break;

		case IDA_TAB2:
			OnSelectTab(1);
			break;

		case IDA_TAB3:
			OnSelectTab(2);
			break;

		case IDA_TAB4:
			OnSelectTab(3);
			break;

		case IDA_TAB5:
			OnSelectTab(4);
			break;

		case IDA_TAB6:
			OnSelectTab(5);
			break;

		case IDA_TAB7:
			OnSelectTab(6);
			break;

		case IDA_TAB8:
			OnSelectTab(7);
			break;

		case IDA_LASTTAB:
			OnSelectTab(-1);
			break;

		case TOOLBAR_VIEWS:
			OnToolbarViews();
			break;

		case TOOLBAR_ADDRESSBAR_GO:
			OnAddressBarGo();
			break;

		/* Messages from the context menu that
		is used with the bookmarks toolbar. */
		case IDM_BT_OPEN:
			BookmarkToolbarOpenItem(m_iSelectedRClick,FALSE);
			break;

		case IDM_BT_OPENINNEWTAB:
			BookmarkToolbarOpenItem(m_iSelectedRClick,TRUE);
			break;

		case IDM_BT_DELETE:
			BookmarkToolbarDeleteItem(m_iSelectedRClick);
			break;

		case IDM_BT_PROPERTIES:
			BookmarkToolbarShowItemProperties(m_iSelectedRClick);
			break;

		case IDM_BT_NEWBOOKMARK:
			BookmarkToolbarNewBookmark(m_iSelectedRClick);
			break;

		case IDM_BT_NEWFOLDER:
			BookmarkToolbarNewFolder(m_iSelectedRClick);
			break;

		case IDM_APP_OPEN:
			ApplicationToolbarOpenItem(m_iSelectedRClick,EMPTY_STRING);
			break;

		case IDM_APP_NEW:
			ApplicationToolbarNewButton();
			break;

		case IDM_APP_DELETE:
			ApplicationToolbarDeleteItem(m_iSelectedRClick);
			break;

		case IDM_APP_PROPERTIES:
			ApplicationToolbarShowItemProperties(m_iSelectedRClick);
			break;

		/* Listview column header context menu. */
		case IDM_HEADER_MORE:
			DialogBoxParam(g_hLanguageModule,MAKEINTRESOURCE(IDD_SELECTCOLUMNS),
				hwnd,SelectColumnsProcStub,(LPARAM)this);
			UpdateArrangeMenuItems();
			break;

		/* Display window menus. */
		case IDM_DW_HIDEDISPLAYWINDOW:
			m_bShowDisplayWindow = FALSE;
			lShowWindow(m_hDisplayWindow,m_bShowDisplayWindow);
			ResizeWindows();
			break;
	}

	switch(HIWORD(wParam))
	{
		case CBN_DROPDOWN:
			AddPathsToComboBoxEx(m_hAddressBar,m_CurrentDirectory);
			break;
	}

	return 1;
}

/*
 * WM_NOTIFY handler for the main window.
 */
LRESULT CALLBACK CContainer::NotifyHandler(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	NMHDR *nmhdr;
	nmhdr = (NMHDR *)lParam;

	switch(nmhdr->code)
	{
		case NM_CLICK:
			if(m_bOneClickActivate)
				OnListViewDoubleClick(&((NMITEMACTIVATE *)lParam)->hdr);
			break;

		case NM_DBLCLK:
			OnListViewDoubleClick(nmhdr);
			break;

		case NM_RCLICK:
			OnRightClick(nmhdr);
			break;

		case NM_CUSTOMDRAW:
			return OnCustomDraw(lParam);
			break;

		case LVN_GETINFOTIP:
			OnListViewGetInfoTip(lParam);
			break;

		case LVN_KEYDOWN:
			return OnListViewKeyDown(lParam);
			break;

		case LVN_ITEMCHANGING:
			{
				UINT uViewMode;
				m_pFolderView[m_iObjectIndex]->GetCurrentViewMode(&uViewMode);
				if(uViewMode == VM_LIST)
				{
					if(m_bBlockNext)
					{
						m_bBlockNext = FALSE;
						return TRUE;
					}
				}
			}
			break;

		case LVN_ITEMCHANGED:
			OnListViewItemChanged(lParam);
			break;

		case LVN_BEGINDRAG:
			OnListViewBeginDrag(lParam,DRAG_TYPE_LEFTCLICK);
			break;

		case LVN_BEGINLABELEDIT:
			return OnListViewBeginLabelEdit(lParam);
			break;

		case LVN_ENDLABELEDIT:
			return OnListViewEndLabelEdit(lParam);
			break;

		case LVN_GETDISPINFO:
			OnListViewGetDisplayInfo(lParam);
			break;

		case LVN_COLUMNCLICK:
			OnListViewColumnClick(lParam);
			break;

		case CBEN_DRAGBEGIN:
			OnAddressBarBeginDrag();
			break;

		case TBN_DROPDOWN:
			OnTbnDropDown(lParam);
			break;

		case TBN_INITCUSTOMIZE:
			return TBNRF_HIDEHELP;
			break;

		case TBN_QUERYINSERT:
			return OnTBQueryInsert(lParam);
			break;

		case TBN_QUERYDELETE:
			return OnTBQueryDelete(lParam);
			break;

		case TBN_GETBUTTONINFO:
			return OnTBGetButtonInfo(lParam);
			break;

		case TBN_RESTORE:
			return OnTBRestore(lParam);
			break;

		case TBN_GETINFOTIP:
			OnTBGetInfoTip(lParam);
			break;

		case TBN_RESET:
			OnTBReset();
			break;

		case TBN_ENDADJUST:
			UpdateToolbarBandSizing(m_hMainRebar,((NMHDR *)lParam)->hwndFrom);
			break;

		case RBN_BEGINDRAG:
			SendMessage(m_hMainRebar,RB_DRAGMOVE,0,-1);
			return 0;
			break;

		case RBN_HEIGHTCHANGE:
			/* The listview and treeview may
			need to be moved to accomodate the new
			rebar size. */
			AdjustFolderPanePosition();
			ResizeWindows();
			break;

		case RBN_CHEVRONPUSHED:
			{
				NMREBARCHEVRON *pnmrc = NULL;
				HWND hToolbar = NULL;
				HMENU hMenu;
				HIMAGELIST himlBackup;
				HIMAGELIST himlSmall;
				MENUITEMINFO mii;
				TCHAR szText[512];
				TBBUTTON tbButton;
				RECT rcToolbar;
				RECT rcButton;
				RECT rcIntersect;
				LRESULT lResult;
				BOOL bIntersect;
				int iMenu = 0;
				int nButtons;
				int i = 0;

				hMenu = CreatePopupMenu();

				pnmrc = (NMREBARCHEVRON *)lParam;

				himlBackup = himlMenu;

				Shell_GetImageLists(NULL,&himlSmall);

				switch(pnmrc->wID)
				{
				case ID_MAINTOOLBAR:
					hToolbar = m_hMainToolbar;
					break;

				case ID_BOOKMARKSTOOLBAR:
					hToolbar = m_hBookmarksToolbar;
					break;

				case ID_DRIVESTOOLBAR:
					hToolbar = m_hDrivesToolbar;
					himlMenu = himlSmall;
					break;

				case ID_APPLICATIONSTOOLBAR:
					hToolbar = m_hApplicationToolbar;
					himlMenu = himlSmall;
					break;
				}

				nButtons = (int)SendMessage(hToolbar,TB_BUTTONCOUNT,0,0);

				GetClientRect(hToolbar,&rcToolbar);

				for(i = 0;i < nButtons;i++)
				{
					lResult = SendMessage(hToolbar,TB_GETITEMRECT,i,(LPARAM)&rcButton);

					if(lResult)
					{
						bIntersect = IntersectRect(&rcIntersect,&rcToolbar,&rcButton);

						if(!bIntersect || rcButton.right > rcToolbar.right)
						{
							SendMessage(hToolbar,TB_GETBUTTON,i,(LPARAM)&tbButton);

							if(tbButton.fsStyle & BTNS_SEP)
							{
								mii.cbSize		= sizeof(mii);
								mii.fMask		= MIIM_FTYPE;
								mii.fType		= MFT_SEPARATOR;
								InsertMenuItem(hMenu,i,TRUE,&mii);
							}
							else
							{
								if(IS_INTRESOURCE(tbButton.iString))
								{
									SendMessage(hToolbar,TB_GETSTRING,MAKEWPARAM(SIZEOF_ARRAY(szText),
										tbButton.iString),(LPARAM)szText);
								}
								else
								{
									StringCchCopy(szText,SIZEOF_ARRAY(szText),(LPCWSTR)tbButton.iString);
								}

								HMENU hSubMenu = NULL;
								UINT fMask;

								fMask = MIIM_ID|MIIM_STRING;
								hSubMenu = NULL;

								switch(pnmrc->wID)
								{
								case ID_MAINTOOLBAR:
									{
										switch(tbButton.idCommand)
										{
										case TOOLBAR_BACK:
											hSubMenu = CreateRebarHistoryMenu(TRUE);
											fMask |= MIIM_SUBMENU;
											break;

										case TOOLBAR_FORWARD:
											hSubMenu = CreateRebarHistoryMenu(FALSE);
											fMask |= MIIM_SUBMENU;
											break;

										case TOOLBAR_VIEWS:
											{
												TCHAR szSubMenuText[512];
												BOOL bRes;
												int nItems;
												int j = 0;

												hSubMenu = CreateMenu();

												nItems = GetMenuItemCount(m_hViewsMenu);

												for(j = 0;j < nItems;j++)
												{
													mii.cbSize		= sizeof(mii);
													mii.fMask		= MIIM_ID|MIIM_STRING;
													mii.dwTypeData	= szSubMenuText;
													mii.cch			= SIZEOF_ARRAY(szSubMenuText);
													bRes = GetMenuItemInfo(m_hViewsMenu,j,TRUE,&mii);

													if(bRes)
													{
														mii.cbSize		= sizeof(mii);
														mii.fMask		= MIIM_ID|MIIM_STRING;
														mii.dwTypeData	= szSubMenuText;
														InsertMenuItem(hSubMenu,j,TRUE,&mii);
													}
												}

												SetMenuOwnerDraw(hSubMenu);

												fMask |= MIIM_SUBMENU;
											}
											break;
										}
									}
									break;

								case ID_BOOKMARKSTOOLBAR:
									{
										Bookmark_t	Bookmark;
										int			iIndex;

										iIndex = (int)SendMessage(m_hBookmarksToolbar,TB_COMMANDTOINDEX,tbButton.idCommand,0);

										if(iIndex != -1)
										{
											SendMessage(m_hBookmarksToolbar,TB_GETBUTTON,iIndex,(LPARAM)&tbButton);

											m_Bookmark.RetrieveBookmark((void *)tbButton.dwData,&Bookmark);

											if(Bookmark.Type == BOOKMARK_TYPE_FOLDER)
											{
												Bookmark_t	ChildBookmark;
												HRESULT		hr;

												hSubMenu = CreateMenu();
												m_hBookmarksMenu = CreateMenu();

												MENUINFO mi;
												mi.cbSize	= sizeof(MENUINFO);
												mi.fMask	= MIM_STYLE;
												mi.dwStyle	= MNS_NOTIFYBYPOS;
												SetMenuInfo(hSubMenu,&mi);

												hr = m_Bookmark.GetChild(&Bookmark,&ChildBookmark);

												if(SUCCEEDED(hr))
												{
													InsertBookmarksIntoMenuInternal(hSubMenu,&ChildBookmark,0,MENU_BOOKMARK_STARTID);
													InsertBookmarksIntoMenuInternal(m_hBookmarksMenu,&ChildBookmark,0,MENU_BOOKMARK_STARTID);
												}
												else
												{
													InsertBookmarksIntoMenuInternal(hSubMenu,NULL,0,MENU_BOOKMARK_STARTID);
													InsertBookmarksIntoMenuInternal(m_hBookmarksMenu,NULL,0,MENU_BOOKMARK_STARTID);
												}

												SetMenuOwnerDraw(hSubMenu);

												fMask |= MIIM_SUBMENU;
											}
										}
									}
									break;
								}

								mii.cbSize		= sizeof(mii);
								mii.fMask		= fMask;
								mii.wID			= tbButton.idCommand;
								mii.hSubMenu	= hSubMenu;
								mii.dwTypeData	= szText;
								InsertMenuItem(hMenu,iMenu,TRUE,&mii);

								SetMenuItemOwnerDrawn(hMenu,iMenu);
								SetMenuItemBitmap(hMenu,tbButton.idCommand,tbButton.iBitmap);	
							}
							iMenu++;
						}
					}
				}

				POINT ptMenu;

				ptMenu.x = pnmrc->rc.left;
				ptMenu.y = pnmrc->rc.bottom;

				ClientToScreen(m_hMainRebar,&ptMenu);

				UINT uFlags = TPM_LEFTALIGN|TPM_RETURNCMD;
				int iCmd;

				iCmd = TrackPopupMenu(hMenu,uFlags,
					ptMenu.x,ptMenu.y,0,m_hMainRebar,NULL);

				if(iCmd != 0)
				{
					/* We'll handle the back and forward buttons
					in place, and send the rest of the messages
					back to the main window. */
					if((iCmd >= ID_REBAR_MENU_BACK_START &&
						iCmd <= ID_REBAR_MENU_BACK_END) ||
						(iCmd >= ID_REBAR_MENU_FORWARD_START &&
						iCmd <= ID_REBAR_MENU_FORWARD_END))
					{
						LPITEMIDLIST pidl = NULL;

						if(iCmd >= ID_REBAR_MENU_BACK_START &&
							iCmd <= ID_REBAR_MENU_BACK_END)
						{
							iCmd = -(iCmd - ID_REBAR_MENU_BACK_START);
						}
						else
						{
							iCmd = iCmd - ID_REBAR_MENU_FORWARD_START;
						}

						pidl = m_pActiveShellBrowser->RetrieveHistoryItem(iCmd);

						BrowseFolder(pidl,SBSP_ABSOLUTE|SBSP_WRITENOHISTORY);

						CoTaskMemFree(pidl);
					}
					else
					{
						SendMessage(m_hContainer,WM_COMMAND,MAKEWPARAM(iCmd,0),0);
					}
				}

				DestroyMenu(hMenu);

				himlMenu = himlBackup;
			}
			break;
	}

	return 0;
}

HMENU CContainer::CreateRebarHistoryMenu(BOOL bBack)
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

/*
 * Processes the specified command line.
 *
 * Command line options:
 * -l	Specifies the language for Explorer++ to use.
 * /?	Causes Explorer++ to show a small help message and exit.
 *
 * Directories can also be passed at any point (no preceeding
 * arguments needed).
 *
 * Paths on the command line are automatically expanded by
 * the shell.
 */
BOOL ProcessCommandLine(TCHAR *pCommandLine)
{
	list<TabDirectory_t>::iterator itr;
	TabDirectory_t TabDirectory;
	TCHAR szPath[MAX_PATH];
	TCHAR *pszCommandLine = NULL;
	BOOL bExit = FALSE;

	g_TabDirs.clear();

	/* The first token will just be the executable name,
	and can be ignored. */
	pszCommandLine = GetToken(pCommandLine,szPath,0);

	while((pszCommandLine = GetToken(pszCommandLine,szPath,0)) != NULL)
	{
		/* Check to see if the user has requested the help page. */
		if(StrCmp(szPath,_T("/?")) == 0)
		{
			/* Show program usage information and then exit. */
			ShowUsage();

			bExit = TRUE;
		}

		if(lstrcmp(szPath,_T("-l")) == 0)
		{
			pszCommandLine = GetToken(pszCommandLine,szPath,0);

			if(pszCommandLine != NULL)
			{
				g_bForceLanguageLoad = TRUE;

				StringCchCopy(g_szLang,SIZEOF_ARRAY(g_szLang),szPath);
			}
		}
		else if(lstrcmp(szPath,_T("-clear_settings")) == 0)
		{
			ClearRegistrySettings();
		}
		else if(lstrcmp(szPath,_T("-remove_as_default")) == 0)
		{
			BOOL bSuccess;

			bSuccess = RemoveAsDefaultFileManagerFileSystem();

			/* Language hasn't been fully specified at this point, so
			can't load success/error message from language dll. Simply show
			a hardcoded success/error message. */
			if(bSuccess)
			{
				MessageBox(NULL,_T("Explorer++ successfully removed as default file manager."),
					WINDOW_NAME,MB_OK);
			}
			else
			{
				MessageBox(NULL,_T("Could not remove Explorer++ as default file manager. Please \
ensure you have administrator privileges."),WINDOW_NAME,MB_ICONWARNING|MB_OK);
			}
		}
		else if(lstrcmp(szPath,_T("-set_as_default")) == 0)
		{
			BOOL bSuccess;

			bSuccess = SetAsDefaultFileManagerFileSystem();

			if(bSuccess)
			{
				MessageBox(NULL,_T("Explorer++ successfully set as default file manager."),
					WINDOW_NAME,MB_OK);
			}
			else
			{
				MessageBox(NULL,_T("Could not set Explorer++ as default file manager. Please \
ensure you have administrator privileges."),WINDOW_NAME,MB_ICONWARNING|MB_OK);
			}
		}
		else if(lstrcmp(szPath,JUMPLIST_TASK_NEWTAB_ARGUMENT) == 0)
		{
			/* This will be called when the user clicks the
			'New Tab' item on the tasks menu in Windows 7.
			Find the already opened version of Explorer++,
			and tell it to open a new tab. */
			HANDLE hMutex;

			hMutex = CreateMutex(NULL,TRUE,_T("Explorer++"));

			if(GetLastError() == ERROR_ALREADY_EXISTS)
			{
				HWND hPrev;

				hPrev = FindWindow(CLASS_NAME,NULL);

				if(hPrev != NULL)
				{
					COPYDATASTRUCT cds;

					cds.cbData	= 0;
					cds.lpData	= NULL;
					SendMessage(hPrev,WM_COPYDATA,NULL,(LPARAM)&cds);

					SetForegroundWindow(hPrev);
					ShowWindow(hPrev,SW_SHOW);
				}
			}

			if(hMutex != NULL)
				CloseHandle(hMutex);

			bExit = TRUE;
		}
		else
		{
			TCHAR szParsingPath[MAX_PATH];
			TCHAR szCurrentDirectory[MAX_PATH];

			GetCurrentProcessImageName(szCurrentDirectory,
				SIZEOF_ARRAY(szCurrentDirectory));
			PathRemoveFileSpec(szCurrentDirectory);

			DecodePath(szPath,szCurrentDirectory,szParsingPath,SIZEOF_ARRAY(szParsingPath));

			StringCchCopy(TabDirectory.Dir,SIZEOF_ARRAY(TabDirectory.Dir),szParsingPath);
			g_TabDirs.push_back(TabDirectory);
		}
	}

	return bExit;
}

void ClearRegistrySettings(void)
{
	LSTATUS lStatus;

	lStatus = SHDeleteKey(HKEY_CURRENT_USER,REG_MAIN_KEY);

	if(lStatus == ERROR_SUCCESS)
		MessageBox(NULL,_T("Settings cleared successfully."),WINDOW_NAME,MB_OK);
	else
		MessageBox(NULL,_T("Settings could not be cleared."),WINDOW_NAME,MB_ICONWARNING);
}

void CContainer::VerifyAndSetWindowPosition(InitialWindowPos_t *piwp)
{
	WINDOWPLACEMENT	wndpl;

	wndpl.length	= sizeof(WINDOWPLACEMENT);
	wndpl.flags		= 0;
	wndpl.showCmd	= SW_HIDE;

	wndpl.rcNormalPosition = piwp->rcNormalPosition;

	if(piwp->bMaximized)
		wndpl.showCmd |= SW_MAXIMIZE;

	SetWindowPlacement(m_hContainer,&wndpl);
}

/*
 * Sets the default position of the main window.
 */
void CContainer::SetDefaultWindowPosition(void)
{
	int	ScreenWidth;
	int	ScreenHeight;
	int	Width;
	int	Height;
	int	Left;
	int	Top;

	ScreenWidth		= GetSystemMetrics(SM_CXSCREEN);
	ScreenHeight	= GetSystemMetrics(SM_CYSCREEN);

	Left	= (int)(DEFAULT_WINDOWPOS_LEFT_PERCENTAGE * ScreenWidth);
	Top		= (int)(DEFAULT_WINDOWPOS_TOP_PERCENTAGE * ScreenHeight);
	Width	= (int)(DEFAULT_WINDOWPOS_WIDTH_PERCENTAGE * ScreenWidth);
	Height	= (int)(DEFAULT_WINDOWPOS_HEIGHT_PERCENTAGE * ScreenHeight);

	SetWindowPos(m_hContainer,NULL,Left,Top,
		Width,Height,SWP_HIDEWINDOW|SWP_NOZORDER);
}

/*
 * Shows a brief message explaining the
 * command line paramters Explorer++
 * uses.
 */
void ShowUsage(void)
{
	TCHAR UsageString[] = _T("Usage:\nexplorer++.exe dir1 dir2 ... dirN\n \
where dir1 to dirN are the directories to \
open.\n\ne.g. explorer++.exe C:\\ D:\\\nwill open the \
directories C:\\ and D:\\, each in their own tabs\n\n\
Virtual folders can be opened simply by \
supplying their name:\n\
e.g. explorer++.exe \"control panel\"\nwill open the \
Control Panel\n");

	MessageBox(NULL,UsageString,WINDOW_NAME,MB_OK);
}

CContainer::CLoadSaveRegistry::CLoadSaveRegistry(CContainer *pContainer)
{
	m_pContainer = pContainer;
}

CContainer::CLoadSaveRegistry::~CLoadSaveRegistry()
{

}

/* IUnknown interface members. */
HRESULT __stdcall CContainer::CLoadSaveRegistry::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = NULL;

	if(*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall CContainer::CLoadSaveRegistry::AddRef(void)
{
	return ++m_iRefCount;
}

ULONG __stdcall CContainer::CLoadSaveRegistry::Release(void)
{
	m_iRefCount--;
	
	if(m_iRefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_iRefCount;
}

HRESULT CContainer::QueryService(REFGUID guidService,REFIID riid,void **ppv)
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

HRESULT CContainer::CreateViewWindow2(LPSV2CVW2_PARAMS lpParams)
{
	return S_OK;
}

HRESULT CContainer::GetView(SHELLVIEWID *pvid,ULONG uView)
{
	return S_OK;
}

HRESULT CContainer::HandleRename(LPCITEMIDLIST pidlNew)
{
	return S_OK;
}

HRESULT CContainer::SelectAndPositionItem(LPCITEMIDLIST pidlItem,UINT uFlags,POINT *ppt)
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

HRESULT CContainer::GetWindow(HWND *)
{
	return S_OK;
}

HRESULT CContainer::ContextSensitiveHelp(BOOL bHelp)
{
	return S_OK;
}

HRESULT CContainer::TranslateAccelerator(MSG *msg)
{
	return S_OK;
}

HRESULT CContainer::EnableModeless(BOOL fEnable)
{
	return S_OK;
}

HRESULT CContainer::UIActivate(UINT uActivate)
{
	return S_OK;
}

HRESULT CContainer::Refresh(void)
{
	return S_OK;
}

HRESULT CContainer::CreateViewWindow(IShellView *psvPrevious,LPCFOLDERSETTINGS pfs,IShellBrowser *psb,RECT *prcView,HWND *phWnd)
{
	return S_OK;
}

HRESULT CContainer::DestroyViewWindow(void)
{
	return S_OK;
}

HRESULT CContainer::GetCurrentInfo(LPFOLDERSETTINGS pfs)
{
	return S_OK;
}

HRESULT CContainer::AddPropertySheetPages(DWORD dwReserved,LPFNSVADDPROPSHEETPAGE pfn,LPARAM lparam)
{
	return S_OK;
}

HRESULT CContainer::SaveViewState(void)
{
	return S_OK;
}

HRESULT CContainer::SelectItem(LPCITEMIDLIST pidlItem,SVSIF uFlags)
{
	return S_OK;
}

HRESULT CContainer::GetItemObject(UINT uItem,REFIID riid,void **ppv)
{
	return S_OK;
}

HRESULT CContainer::IncludeItems(NMCII_FLAGS *pFlags)
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

HRESULT CContainer::SelectAndEditItem(PCIDLIST_ABSOLUTE pidlItem,NMCSAEI_FLAGS flags)
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