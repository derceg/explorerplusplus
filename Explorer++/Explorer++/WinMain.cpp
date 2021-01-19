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
#include "XMLSettings.h"
#include "../Helper/Logging.h"
#include "../Helper/Macros.h"
#include "../Helper/ProcessHelper.h"
#include "../ThirdParty/CLI11/CLI11.hpp"
#include <boost/format.hpp>
#include <boost/locale.hpp>
#include <boost/scope_exit.hpp>
#include <wil/resource.h>

#pragma warning(disable:4459) // declaration of 'boost_scope_exit_aux_args' hides global declaration

/* Default window size/position. */
#define DEFAULT_WINDOWPOS_LEFT_PERCENTAGE	0.02
#define DEFAULT_WINDOWPOS_TOP_PERCENTAGE	0.05
#define DEFAULT_WINDOWPOS_WIDTH_PERCENTAGE	0.96
#define DEFAULT_WINDOWPOS_HEIGHT_PERCENTAGE	0.82

ATOM RegisterMainWindowClass(HINSTANCE hInstance);
LONG WINAPI TopLevelExceptionFilter(EXCEPTION_POINTERS *exception);
void InitializeLocale();

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
bool g_registerForShellNotifications = false;

ATOM RegisterMainWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	wcex.cbSize			= sizeof(wcex);
	wcex.style			= 0;
	wcex.lpfnWndProc	= Explorerplusplus::WndProcStub;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= sizeof(Explorerplusplus *);
	wcex.hInstance		= hInstance;
	wcex.hIcon			= (HICON)LoadImage(hInstance,MAKEINTRESOURCE(IDI_MAIN),IMAGE_ICON,GetSystemMetrics(SM_CXICON),GetSystemMetrics(SM_CYICON),LR_DEFAULTCOLOR);
	wcex.hIconSm		= (HICON)LoadImage(hInstance,MAKEINTRESOURCE(IDI_MAIN),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_DEFAULTCOLOR);
	wcex.hCursor		= LoadCursor(nullptr,IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)nullptr;
	wcex.lpszMenuName	= nullptr;
	wcex.lpszClassName	= NExplorerplusplus::CLASS_NAME;
	return RegisterClassEx(&wcex);
}

LONG WINAPI TopLevelExceptionFilter(EXCEPTION_POINTERS *exception)
{
	TCHAR currentProcess[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), currentProcess, SIZEOF_ARRAY(currentProcess));

	// Event names are global in the system. Therefore, the event name used for signaling should be
	// unique.
	auto eventName = CreateGUID();
	wil::unique_event_nothrow event;
	bool res = event.try_create(wil::EventOptions::ManualReset, eventName.c_str());

	if (!res)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	// Note that while %#p should print the base (i.e. 0x), that doesn't appear to work. That's why
	// the 0x prefix is manually set in the string below. It's important that the base is set so
	// that the value is correctly interpreted as a hex value.
	std::wstring arguments = (boost::wformat(L"\"%s\" %s %lu %lu 0x%p %s") % currentProcess
		% NExplorerplusplus::APPLICATION_CRASHED_ARGUMENT % GetCurrentProcessId()
		% GetCurrentThreadId() % exception % eventName)
								 .str();

	STARTUPINFO startupInfo = { 0 };
	startupInfo.cb = sizeof(startupInfo);
	wil::unique_process_information processInformation;
	res = CreateProcess(currentProcess, arguments.data(), nullptr, nullptr, false,
		NORMAL_PRIORITY_CLASS, nullptr, nullptr, &startupInfo, &processInformation);

	if (!res)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	// The process that's created above will attempt to create a minidump for this process. It's
	// possible that the new process will itself crash for whatever reason, so the wait here
	// shouldn't be indefinite.
	event.wait(30000);

	return EXCEPTION_EXECUTE_HANDLER;
}

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	bool enableLogging =
#ifdef _DEBUG
		true;
#else
		false;
#endif

	boost::log::core::get()->set_logging_enabled(enableLogging);

	/* Initialize OLE, as well as the various window classes that
	will be needed (listview, TreeView, comboboxex, etc.). */
	INITCOMMONCONTROLSEX	ccEx;
	ccEx.dwSize	= sizeof(INITCOMMONCONTROLSEX);
	ccEx.dwICC	= dwControlClasses;
	InitCommonControlsEx(&ccEx);

	auto oleCleanup = wil::OleInitialize_failfast();

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::Status status = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

	if (status != Gdiplus::Status::Ok)
	{
		return 0;
	}

	auto gdiplusCleanup = wil::scope_exit([gdiplusToken] {
		Gdiplus::GdiplusShutdown(gdiplusToken);
	});

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
	if(!g_commandLineDirectories.empty())
	{
		unique_pidl_absolute pidlControlPanel;
		HRESULT hr = SHGetFolderLocation(nullptr,
			CSIDL_CONTROLS, nullptr,0,wil::out_param(pidlControlPanel));

		if(SUCCEEDED(hr))
		{
			auto itr = g_commandLineDirectories.begin();

			BOOL bControlPanelChild = FALSE;

			while(itr != g_commandLineDirectories.end())
			{
				/* This could fail on a 64-bit version of Windows if the
				executable is 32-bit, and the folder is 64-bit specific (as is
				the case with some of the folders under the control panel). */
				unique_pidl_absolute pidl;
				hr = SHParseDisplayName(itr->c_str(), nullptr, wil::out_param(pidl), 0, nullptr);

				bControlPanelChild = FALSE;

				if(SUCCEEDED(hr))
				{
					if(ILIsParent(pidlControlPanel.get(),pidl.get(),FALSE) &&
						!ArePidlsEquivalent(pidlControlPanel.get(),pidl.get()))
					{
						bControlPanelChild = TRUE;
					}
					else
					{
						unique_pidl_absolute pidlControlPanelCategory;
						hr = SHParseDisplayName(CONTROL_PANEL_CATEGORY_VIEW, nullptr,
							wil::out_param(pidlControlPanelCategory), 0, nullptr);

						if (SUCCEEDED(hr))
						{
							if (ILIsParent(pidlControlPanelCategory.get(), pidl.get(), FALSE) &&
								!ArePidlsEquivalent(pidlControlPanelCategory.get(), pidl.get()))
							{
								bControlPanelChild = TRUE;
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
						ShellExecute(nullptr,_T("open"),szExplorerPath,
							itr->c_str(), nullptr,SW_SHOWNORMAL);

						itr = g_commandLineDirectories.erase(itr);
					}
				}


				if(!bControlPanelChild)
				{
					itr++;
				}
			}

			if(g_commandLineDirectories.empty())
			{
				shouldExit = true;
			}
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
	wil::unique_mutex_nothrow applicationMutex(CreateMutex(nullptr,TRUE,_T("Explorer++")));

	if(!bAllowMultipleInstances)
	{
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			HWND hPrev;

			hPrev = FindWindow(NExplorerplusplus::CLASS_NAME, nullptr);

			if(hPrev != nullptr)
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
					cds.lpData	= nullptr;
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
		MessageBox(nullptr,_T("Could not register class"),NExplorerplusplus::APP_NAME,
			MB_OK|MB_ICONERROR);

		return 0;
	}

	SetUnhandledExceptionFilter(TopLevelExceptionFilter);

	InitializeLocale();

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
	nullptr,
	nullptr,
	hInstance,
	nullptr);

	if(hwnd == nullptr)
	{
		MessageBox(nullptr,_T("Could not create main window."),NExplorerplusplus::APP_NAME,
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

	g_hwndSearch = nullptr;
	g_hwndRunScript = nullptr;
	g_hwndOptions = nullptr;
	g_hwndManageBookmarks = nullptr;

	MSG msg;

	/* Enter the message loop... */
	while(GetMessage(&msg, nullptr,0,0) > 0)
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

		if(g_hwndOptions && PropSheet_GetCurrentPageHwnd(g_hwndOptions) == nullptr)
		{
			DestroyWindow(g_hwndOptions);
			g_hwndOptions = nullptr;
		}
	}

	return (int)msg.wParam;
}

void InitializeLocale()
{
	auto backendManager = boost::locale::localization_backend_manager::global();
	backendManager.select("winapi");
	boost::locale::localization_backend_manager::global(backendManager);

	// Use the system default locale.
	boost::locale::generator gen;
	std::locale::global(gen(""));
}