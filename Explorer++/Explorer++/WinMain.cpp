/******************************************************************
 *
 * Project: Explorer++
 * File: Settings.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Explorer++ is an Explorer-like file manager for
 * Windows XP, Vista and 7.
 *
 * This is the main module for Explorer++. Handles startup.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/SetDefaultFileManager.h"


typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(
	HANDLE hProcee,DWORD ProcessId,HANDLE hFile,MINIDUMP_TYPE DumpType,
	PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

BOOL ProcessCommandLine(TCHAR *pCommandLine);
void ShowUsage(void);
void ClearRegistrySettings(void);
ATOM RegisterMainWindowClass(void);
LONG WINAPI MyUnhandledExceptionFilter(struct _EXCEPTION_POINTERS *pExceptionInfo);

extern LRESULT CALLBACK WndProcStub(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);

DWORD dwControlClasses = ICC_BAR_CLASSES|ICC_COOL_CLASSES|
	ICC_LISTVIEW_CLASSES|ICC_USEREX_CLASSES|ICC_STANDARD_CLASSES|
	ICC_LINK_CLASS;
list<std::wstring> g_TabDirs;

/* Search and options dialogs. */
HWND g_hwndSearch;
HWND g_hwndOptions;

HINSTANCE g_hLanguageModule;
TCHAR g_szLang[32];
BOOL g_bForceLanguageLoad = FALSE;

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

			bSuccess = NDefaultFileManager::RemoveAsDefaultFileManagerFileSystem(SHELL_DEFAULT_INTERNAL_COMMAND_NAME);

			/* Language hasn't been fully specified at this point, so
			can't load success/error message from language dll. Simply show
			a hardcoded success/error message. */
			if(bSuccess)
			{
				MessageBox(NULL,_T("Explorer++ successfully removed as default file manager."),
					NExplorerplusplus::WINDOW_NAME,MB_OK);
			}
			else
			{
				MessageBox(NULL,_T("Could not remove Explorer++ as default file manager. Please \
ensure you have administrator privileges."),NExplorerplusplus::WINDOW_NAME,MB_ICONWARNING|MB_OK);
			}
		}
		else if(lstrcmp(szPath,_T("-set_as_default")) == 0)
		{
			BOOL bSuccess;

			bSuccess = NDefaultFileManager::SetAsDefaultFileManagerFileSystem(SHELL_DEFAULT_INTERNAL_COMMAND_NAME,
				SHELL_DEFAULT_MENU_TEXT);

			if(bSuccess)
			{
				MessageBox(NULL,_T("Explorer++ successfully set as default file manager."),
					NExplorerplusplus::WINDOW_NAME,MB_OK);
			}
			else
			{
				MessageBox(NULL,_T("Could not set Explorer++ as default file manager. Please \
ensure you have administrator privileges."),NExplorerplusplus::WINDOW_NAME,MB_ICONWARNING|MB_OK);
			}
		}
		else if(lstrcmp(szPath,NExplorerplusplus::JUMPLIST_TASK_NEWTAB_ARGUMENT) == 0)
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

				hPrev = FindWindow(NExplorerplusplus::CLASS_NAME,NULL);

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
			g_TabDirs.push_back(szParsingPath);
		}
	}

	return bExit;
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

	MessageBox(NULL,UsageString,NExplorerplusplus::WINDOW_NAME,MB_OK);
}

void ClearRegistrySettings(void)
{
	LSTATUS lStatus;

	lStatus = SHDeleteKey(HKEY_CURRENT_USER,REG_MAIN_KEY);

	if(lStatus == ERROR_SUCCESS)
		MessageBox(NULL,_T("Settings cleared successfully."),NExplorerplusplus::WINDOW_NAME,MB_OK);
	else
		MessageBox(NULL,_T("Settings could not be cleared."),NExplorerplusplus::WINDOW_NAME,MB_ICONWARNING);
}

/*
 * Registers the main window class.
 */
ATOM RegisterMainWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	/* Prepare the structure needed to create the main window. */
	wcex.cbSize			= sizeof(wcex);
	wcex.style			= 0;
	wcex.lpfnWndProc	= WndProcStub;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= sizeof(Explorerplusplus *);
	wcex.hInstance		= hInstance;
	wcex.hIcon			= (HICON)LoadImage(hInstance,MAKEINTRESOURCE(IDI_MAIN),IMAGE_ICON,48,48,LR_VGACOLOR);
	wcex.hIconSm		= (HICON)LoadImage(hInstance,MAKEINTRESOURCE(IDI_MAIN_SMALL),IMAGE_ICON,16,16,LR_VGACOLOR);
	wcex.hCursor		= LoadCursor(NULL,IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)NULL;
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= NExplorerplusplus::CLASS_NAME;

	return RegisterClassEx(&wcex);
}

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
				szPath,szAppName,NExplorerplusplus::VERSION_NUMBER,stLocalTime.wDay,stLocalTime.wMonth,
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
	InitControlClasses(dwControlClasses);
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
				hr = GetIdlFromParsingName(itr->c_str(),&pidl);

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
							itr->c_str(),NULL,SW_SHOWNORMAL);

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
		bAllowMultipleInstances = LoadAllowMultipleInstancesFromRegistry();
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

			hPrev = FindWindow(NExplorerplusplus::CLASS_NAME,NULL);

			if(hPrev != NULL)
			{
				if(!g_TabDirs.empty())
				{
					for each(auto strDirectory in g_TabDirs)
					{
						COPYDATASTRUCT cds;
						TCHAR szDirectory[MAX_PATH];

						StringCchCopy(szDirectory,SIZEOF_ARRAY(szDirectory),strDirectory.c_str());

						cds.cbData	= static_cast<DWORD>((strDirectory.size() + 1) * sizeof(TCHAR));
						cds.lpData	= szDirectory;
						SendMessage(hPrev,WM_COPYDATA,NULL,reinterpret_cast<LPARAM>(&cds));
					}
				}
				else
				{
					COPYDATASTRUCT cds;

					cds.cbData	= 0;
					cds.lpData	= NULL;
					SendMessage(hPrev,WM_COPYDATA,NULL,reinterpret_cast<LPARAM>(&cds));
				}

				SetForegroundWindow(hPrev);
				ShowWindow(hPrev,SW_RESTORE);
				CloseHandle(hMutex);
				return 0;
			}
		}
	}

	/* This dll is needed to create a richedit control. */
	hRichEditLib = LoadLibrary(_T("Riched20.dll"));

	res = RegisterMainWindowClass(hInstance);

	if(res == 0)
	{
		MessageBox(NULL,_T("Could not register class"),NExplorerplusplus::WINDOW_NAME,
			MB_OK|MB_ICONERROR);

		FreeLibrary(hRichEditLib);
		OleUninitialize();

		return 0;
	}

	SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);

	/* Create the main window. This window will act as a
	container for all child windows created. */
	hwnd = CreateWindow(
	NExplorerplusplus::CLASS_NAME,
	NExplorerplusplus::WINDOW_NAME,
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
		MessageBox(NULL,_T("Could not create main window."),NExplorerplusplus::WINDOW_NAME,
			MB_OK|MB_ICONERROR);

		FreeLibrary(hRichEditLib);
		OleUninitialize();

		return 0;
	}

	WINDOWPLACEMENT	wndpl;
	BOOL bWindowPosLoaded = FALSE;

	if(bLoadSettingsFromXML)
	{
		bWindowPosLoaded = LoadWindowPositionFromXML(&wndpl);
	}
	else
	{
		bWindowPosLoaded = LoadWindowPosition(&wndpl);
	}

	/* If no window position was loaded, use
	the default position. */
	if(!bWindowPosLoaded)
	{
		wndpl.length = sizeof(wndpl);
		wndpl.showCmd = nCmdShow;
		wndpl.flags = 0;

		wndpl.ptMinPosition.x = 0;
		wndpl.ptMinPosition.y = 0;
		wndpl.ptMaxPosition.x = -1;
		wndpl.ptMaxPosition.y = -1;

		int iScreenWidth = GetSystemMetrics(SM_CXSCREEN);
		int iScreenHeight = GetSystemMetrics(SM_CYSCREEN);

		wndpl.rcNormalPosition.left = (LONG)(DEFAULT_WINDOWPOS_LEFT_PERCENTAGE * iScreenWidth);
		wndpl.rcNormalPosition.top = (LONG)(DEFAULT_WINDOWPOS_TOP_PERCENTAGE * iScreenHeight);

		wndpl.rcNormalPosition.right = wndpl.rcNormalPosition.left +
			(LONG)(DEFAULT_WINDOWPOS_WIDTH_PERCENTAGE * iScreenWidth);
		wndpl.rcNormalPosition.bottom = wndpl.rcNormalPosition.top +
			(LONG)(DEFAULT_WINDOWPOS_HEIGHT_PERCENTAGE * iScreenHeight);
	}

	/* If the incoming state (nCmdShow) is minimized
	or maximized, use that state, instead of the
	saved state. */
	if(nCmdShow == SW_MINIMIZE ||
		nCmdShow == SW_SHOWMINIMIZED ||
		nCmdShow == SW_MAXIMIZE)
	{
		wndpl.showCmd = nCmdShow;
	}

	SetWindowPlacement(hwnd,&wndpl);
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