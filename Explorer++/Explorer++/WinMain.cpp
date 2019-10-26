// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * This is the main module for Explorer++. Handles startup.
 */

#include "stdafx.h"
#include "Explorer++.h"
#include "CommandLine.h"
#include "Console.h"
#include "Explorer++_internal.h"
#include "Logging.h"
#include "MainResource.h"
#include "ModelessDialogs.h"
#include "RegistrySettings.h"
#include "Version.h"
#include "XMLSettings.h"
#include "../Helper/Logging.h"
#include "../Helper/Macros.h"
#include "../Helper/ProcessHelper.h"
#include "../ThirdParty/CLI11/CLI11.hpp"
#include <boost/scope_exit.hpp>
#include <wil/resource.h>

#pragma warning(disable:4459) // declaration of 'boost_scope_exit_aux_args' hides global declaration

/* Default window size/position. */
#define DEFAULT_WINDOWPOS_LEFT_PERCENTAGE	0.02
#define DEFAULT_WINDOWPOS_TOP_PERCENTAGE	0.05
#define DEFAULT_WINDOWPOS_WIDTH_PERCENTAGE	0.96
#define DEFAULT_WINDOWPOS_HEIGHT_PERCENTAGE	0.82

typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(
	HANDLE hProcee,DWORD ProcessId,HANDLE hFile,MINIDUMP_TYPE DumpType,
	PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

ATOM RegisterMainWindowClass(HINSTANCE hInstance);
LONG WINAPI MyUnhandledExceptionFilter(struct _EXCEPTION_POINTERS *pExceptionInfo);

DWORD dwControlClasses = ICC_BAR_CLASSES|ICC_COOL_CLASSES|
	ICC_LISTVIEW_CLASSES|ICC_USEREX_CLASSES|ICC_STANDARD_CLASSES|
	ICC_LINK_CLASS;
std::vector<std::wstring> g_commandLineDirectories;

/* Modeless dialog handles. */
HWND g_hwndSearch;
HWND g_hwndRunScript;
HWND g_hwndOptions;
HWND g_hwndManageBookmarks;

TCHAR g_szLang[32];
BOOL g_bForceLanguageLoad = FALSE;

HACCEL g_hAccl;

bool g_enablePlugins = false;

ATOM RegisterMainWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	wcex.cbSize			= sizeof(wcex);
	wcex.style			= 0;
	wcex.lpfnWndProc	= WndProcStub;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= sizeof(Explorerplusplus *);
	wcex.hInstance		= hInstance;
	wcex.hIcon			= (HICON)LoadImage(hInstance,MAKEINTRESOURCE(IDI_MAIN),IMAGE_ICON,GetSystemMetrics(SM_CXICON),GetSystemMetrics(SM_CYICON),LR_DEFAULTCOLOR);
	wcex.hIconSm		= (HICON)LoadImage(hInstance,MAKEINTRESOURCE(IDI_MAIN),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_DEFAULTCOLOR);
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
				szPath,NExplorerplusplus::APP_NAME,VERSION_STRING_W,stLocalTime.wDay,stLocalTime.wMonth,
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

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);

	/* This command line string is
	ANSI, so GetCommandLine is used
	below to retrieve the Unicode
	version. */
	UNREFERENCED_PARAMETER(lpCmdLine);

	bool enableLogging =
#ifdef _DEBUG
		true;
#else
		false;
#endif;

	boost::log::core::get()->set_logging_enabled(enableLogging);

	/* Initialize OLE, as well as the various window classes that
	will be needed (listview, TreeView, comboboxex, etc.). */
	INITCOMMONCONTROLSEX	ccEx;
	ccEx.dwSize	= sizeof(INITCOMMONCONTROLSEX);
	ccEx.dwICC	= dwControlClasses;
	InitCommonControlsEx(&ccEx);

	auto oleCleanup = wil::OleInitialize_failfast();

	bool consoleAttached = Console::AttachParentConsole();

	BOOST_SCOPE_EXIT(consoleAttached) {
		if (consoleAttached)
		{
			Console::ReleaseConsole();
		}
	} BOOST_SCOPE_EXIT_END

	auto exitInfo = CommandLine::ProcessCommandLine();

	if (exitInfo)
	{
		return exitInfo->exitCode;
	}

	InitializeLogging(NExplorerplusplus::LOG_FILENAME);

	bool shouldExit = false;

	/* Can't open folders that are children of the
	control panel. If the command line only refers
	to folders that are children of the control panel,
	pass those folders to Windows Explorer, then exit. */
	if(g_commandLineDirectories.size() > 0)
	{
		LPITEMIDLIST pidlControlPanel = NULL;
		LPITEMIDLIST pidl = NULL;

		HRESULT hr = SHGetFolderLocation(NULL,
			CSIDL_CONTROLS,NULL,0,&pidlControlPanel);

		if(SUCCEEDED(hr))
		{
			auto itr = g_commandLineDirectories.begin();

			BOOL bControlPanelChild = FALSE;

			while(itr != g_commandLineDirectories.end())
			{
				/* This could fail on a 64-bit version of Windows if the
				executable is 32-bit, and the folder is 64-bit specific (as is
				the case with some of the folders under the control panel). */
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
						LPITEMIDLIST pidlControlPanelCategory = NULL;

						hr = GetIdlFromParsingName(CONTROL_PANEL_CATEGORY_VIEW,
							&pidlControlPanelCategory);

						if (SUCCEEDED(hr))
						{
							if (ILIsParent(pidlControlPanelCategory, pidl, FALSE) &&
								!CompareIdls(pidlControlPanelCategory, pidl))
							{
								bControlPanelChild = TRUE;
							}

							CoTaskMemFree(pidlControlPanelCategory);
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

						itr = g_commandLineDirectories.erase(itr);
					}

					CoTaskMemFree(pidl);
				}


				if(!bControlPanelChild)
				{
					itr++;
				}
			}

			if(g_commandLineDirectories.size() == 0)
			{
				shouldExit = true;
			}

			CoTaskMemFree(pidlControlPanel);
		}
	}

	if(shouldExit)
	{
		return 0;
	}

	BOOL bAllowMultipleInstances = TRUE;
	BOOL bLoadSettingsFromXML;

	bLoadSettingsFromXML = TestConfigFileInternal();
	LOG(info) << _T("bLoadSettingsFromXML = ") << bLoadSettingsFromXML;

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
	wil::unique_mutex applicationMutex(CreateMutex(NULL,TRUE,_T("Explorer++")));

	if(!bAllowMultipleInstances)
	{
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			HWND hPrev;

			hPrev = FindWindow(NExplorerplusplus::CLASS_NAME,NULL);

			if(hPrev != NULL)
			{
				if(!g_commandLineDirectories.empty())
				{
					for(const auto &strDirectory : g_commandLineDirectories)
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
				return 0;
			}
		}
	}

	/* This dll is needed to create a richedit control. */
	wil::unique_hmodule richEditLib(LoadLibrary(_T("Riched20.dll")));

	LONG res = RegisterMainWindowClass(hInstance);

	if(res == 0)
	{
		MessageBox(NULL,_T("Could not register class"),NExplorerplusplus::APP_NAME,
			MB_OK|MB_ICONERROR);

		return 0;
	}

	SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);

	g_hAccl = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_MAINACCELERATORS));

	/* Create the main window. This window will act as a
	container for all child windows created. */
	HWND hwnd = CreateWindow(
	NExplorerplusplus::CLASS_NAME,
	NExplorerplusplus::APP_NAME,
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
		MessageBox(NULL,_T("Could not create main window."),NExplorerplusplus::APP_NAME,
			MB_OK|MB_ICONERROR);

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
		bWindowPosLoaded = LoadWindowPositionFromRegistry(&wndpl);
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

	g_hwndSearch = NULL;
	g_hwndRunScript = NULL;
	g_hwndOptions = NULL;
	g_hwndManageBookmarks = NULL;

	MSG msg;

	/* Enter the message loop... */
	while(GetMessage(&msg,NULL,0,0) > 0)
	{
		/* TranslateMessage() must be in the inner loop,
		otherwise various accelerator keys (such as tab)
		would be taken even when the dialog has focus. */
		if(!IsDialogMessage(g_hwndSearch,&msg) &&
			!IsDialogMessage(g_hwndManageBookmarks,&msg) &&
			!IsDialogMessage(g_hwndRunScript, &msg) &&
			!PropSheet_IsDialogMessage(g_hwndOptions,&msg))
		{
			if(!TranslateAccelerator(hwnd, g_hAccl,&msg))
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

	return (int)msg.wParam;
}