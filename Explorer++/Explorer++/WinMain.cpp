// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * This is the main module for Explorer++. Handles startup.
 */

#include "stdafx.h"
#include "Explorer++.h"
#include "AcceleratorManager.h"
#include "App.h"
#include "CommandLine.h"
#include "CrashHandlerHelper.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "ModelessDialogs.h"
#include "RegistrySettings.h"
#include "XMLSettings.h"
#include "../Helper/Macros.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/WindowHelper.h"
#include <boost/locale.hpp>
#include <wil/resource.h>
#include <cstdlib>
#include <format>

/* Default window size/position. */
#define DEFAULT_WINDOWPOS_LEFT_PERCENTAGE 0.02
#define DEFAULT_WINDOWPOS_TOP_PERCENTAGE 0.05
#define DEFAULT_WINDOWPOS_WIDTH_PERCENTAGE 0.96
#define DEFAULT_WINDOWPOS_HEIGHT_PERCENTAGE 0.82

ATOM RegisterMainWindowClass(HINSTANCE hInstance);
[[nodiscard]] unique_glog_shutdown_call InitializeLogging(
	const CommandLine::Settings *commandLineSettings);
void InitializeLocale();

DWORD dwControlClasses = ICC_BAR_CLASSES | ICC_COOL_CLASSES | ICC_LISTVIEW_CLASSES
	| ICC_USEREX_CLASSES | ICC_STANDARD_CLASSES | ICC_LINK_CLASS;

/* Modeless dialog handles. */
HWND g_hwndSearch = nullptr;
HWND g_hwndRunScript = nullptr;
HWND g_hwndOptions = nullptr;
HWND g_hwndManageBookmarks = nullptr;
HWND g_hwndSearchTabs = nullptr;

ATOM RegisterMainWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(wcex);
	wcex.style = 0;
	wcex.lpfnWndProc = Explorerplusplus::WndProcStub;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = sizeof(Explorerplusplus *);
	wcex.hInstance = hInstance;
	wcex.hIcon = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON,
		GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	wcex.hIconSm = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH) nullptr;
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = NExplorerplusplus::CLASS_NAME;
	return RegisterClassEx(&wcex);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	/* Initialize OLE, as well as the various window classes that
	will be needed (listview, TreeView, comboboxex, etc.). */
	INITCOMMONCONTROLSEX ccEx;
	ccEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
	ccEx.dwICC = dwControlClasses;
	InitCommonControlsEx(&ccEx);

	auto oleCleanup = wil::OleInitialize_failfast();

	auto consoleCleanup = AttachParentConsole();

	auto commandLineInfo = CommandLine::ProcessCommandLine();

	if (std::holds_alternative<CommandLine::ExitInfo>(commandLineInfo))
	{
		return std::get<CommandLine::ExitInfo>(commandLineInfo).exitCode;
	}

	auto &commandLineSettings = std::get<CommandLine::Settings>(commandLineInfo);

	auto glogCleanup = InitializeLogging(&commandLineSettings);

	BOOL bAllowMultipleInstances = TRUE;
	BOOL bLoadSettingsFromXML;

	bLoadSettingsFromXML = TestConfigFileInternal();

	if (bLoadSettingsFromXML)
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
	wil::unique_mutex_nothrow applicationMutex(
		CreateMutex(nullptr, TRUE, NExplorerplusplus::APPLICATION_MUTEX_NAME));

	if (!bAllowMultipleInstances)
	{
		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			HWND hPrev;

			hPrev = FindWindow(NExplorerplusplus::CLASS_NAME, nullptr);

			if (hPrev != nullptr)
			{
				if (!commandLineSettings.directories.empty())
				{
					for (const auto &strDirectory : commandLineSettings.directories)
					{
						COPYDATASTRUCT cds;
						TCHAR szDirectory[MAX_PATH];

						StringCchCopy(szDirectory, std::size(szDirectory), strDirectory.c_str());

						cds.cbData = static_cast<DWORD>((strDirectory.size() + 1) * sizeof(TCHAR));
						cds.lpData = szDirectory;
						SendMessage(hPrev, WM_COPYDATA, NULL, reinterpret_cast<LPARAM>(&cds));
					}
				}
				else
				{
					COPYDATASTRUCT cds;

					cds.cbData = 0;
					cds.lpData = nullptr;
					SendMessage(hPrev, WM_COPYDATA, NULL, reinterpret_cast<LPARAM>(&cds));
				}

				BringWindowToForeground(hPrev);
				return EXIT_CODE_NORMAL_EXISTING_PROCESS;
			}
		}
	}

	/* This dll is needed to create a richedit control. */
	wil::unique_hmodule richEditLib(LoadLibrary(_T("Msftedit.dll")));

	LONG res = RegisterMainWindowClass(hInstance);

	if (res == 0)
	{
		MessageBox(nullptr, _T("Could not register class"), NExplorerplusplus::APP_NAME,
			MB_OK | MB_ICONERROR);

		return EXIT_CODE_ERROR;
	}

	InitializeCrashHandler();

	InitializeLocale();

	App app(&commandLineSettings);

	/* Create the main window. This window will act as a
	container for all child windows created. */
	HWND hwnd = CreateWindow(NExplorerplusplus::CLASS_NAME, NExplorerplusplus::APP_NAME,
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr,
		nullptr, hInstance, &app);

	if (hwnd == nullptr)
	{
		MessageBox(nullptr, _T("Could not create main window."), NExplorerplusplus::APP_NAME,
			MB_OK | MB_ICONERROR);

		return EXIT_CODE_ERROR;
	}

	WINDOWPLACEMENT wndpl;
	BOOL bWindowPosLoaded = FALSE;

	if (bLoadSettingsFromXML)
	{
		bWindowPosLoaded = LoadWindowPositionFromXML(&wndpl);
	}
	else
	{
		bWindowPosLoaded = LoadWindowPositionFromRegistry(&wndpl);
	}

	if (bWindowPosLoaded)
	{
		// When shown in its normal size, the window for the application should at least be on
		// screen somewhere, even if it's not completely visible. Therefore, the position should be
		// reset if the window won't be visible on any monitor.
		// Checking this on startup makes sense, since the monitor setup can change in between
		// executions.
		HMONITOR monitor = MonitorFromRect(&wndpl.rcNormalPosition, MONITOR_DEFAULTTONULL);

		if (!monitor)
		{
			bWindowPosLoaded = FALSE;
		}
	}

	/* If no window position was loaded, use
	the default position. */
	if (!bWindowPosLoaded)
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

		wndpl.rcNormalPosition.left = (LONG) (DEFAULT_WINDOWPOS_LEFT_PERCENTAGE * iScreenWidth);
		wndpl.rcNormalPosition.top = (LONG) (DEFAULT_WINDOWPOS_TOP_PERCENTAGE * iScreenHeight);

		wndpl.rcNormalPosition.right = wndpl.rcNormalPosition.left
			+ (LONG) (DEFAULT_WINDOWPOS_WIDTH_PERCENTAGE * iScreenWidth);
		wndpl.rcNormalPosition.bottom = wndpl.rcNormalPosition.top
			+ (LONG) (DEFAULT_WINDOWPOS_HEIGHT_PERCENTAGE * iScreenHeight);
	}

	/* If the incoming state (nCmdShow) is minimized
	or maximized, use that state, instead of the
	saved state. */
	if (nCmdShow == SW_MINIMIZE || nCmdShow == SW_SHOWMINIMIZED || nCmdShow == SW_MAXIMIZE)
	{
		wndpl.showCmd = nCmdShow;
	}

	SetWindowPlacement(hwnd, &wndpl);
	UpdateWindow(hwnd);

	MSG msg;

	/* Enter the message loop... */
	while (GetMessage(&msg, nullptr, 0, 0) > 0)
	{
		/* TranslateMessage() must be in the inner loop,
		otherwise various accelerator keys (such as tab)
		would be taken even when the dialog has focus. */
		if (!IsDialogMessage(g_hwndSearch, &msg) && !IsDialogMessage(g_hwndManageBookmarks, &msg)
			&& !IsDialogMessage(g_hwndRunScript, &msg) && !IsDialogMessage(g_hwndOptions, &msg)
			&& !IsDialogMessage(g_hwndSearchTabs, &msg))
		{
			if (!TranslateAccelerator(hwnd, app.GetAcceleratorManager()->GetAcceleratorTable(),
					&msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	return (int) msg.wParam;
}

unique_glog_shutdown_call InitializeLogging(const CommandLine::Settings *commandLineSettings)
{
	if (!commandLineSettings->enableLogging)
	{
		// Logs will only go to stdout. This will effectively disable logging from the user's
		// perspective.
		FLAGS_logtostdout = 1;

		// Doing this means that, if the application was launched from the console, for example,
		// CHECK failures will still be shown, which is somewhat useful.
		FLAGS_minloglevel = google::GLOG_ERROR;
	}

	auto glogCleanup = InitializeGoogleLogging();

	google::InstallFailureFunction(
		[]
		{
			if (!IsDebuggerPresent())
			{
				// By default, glog will call abort() on a CHECK failure. The issue with that is
				// that it won't invoke the exception handler. By calling DebugBreak() instead, the
				// exception handler will be called, which gives a chance for the crash dialog to be
				// shown and a minidump to be generated, before the application is terminated.
				// This is only done when not debugging. When the application is being debugged,
				// calling DebugBreak() would simply cause the debugger to break, which isn't
				// desirable, as the application should exit when a CHECK condition is violated.
				DebugBreak();
			}
			else
			{
				std::abort();
			}
		});

	wil::SetResultLoggingCallback(
		[](const wil::FailureInfo &failure) noexcept
		{
			WCHAR logMessage[2048];
			HRESULT hr = wil::GetFailureLogString(logMessage, std::size(logMessage), failure);

			if (FAILED(hr))
			{
				return;
			}

			auto logMessageUtf8 = wstrToUtf8Str(logMessage);

			if (failure.type == wil::FailureType::FailFast)
			{
				// In this case, a fatal error has been triggered and WIL will terminate the
				// application after returning.
				LOG(ERROR) << logMessageUtf8;
			}
			else
			{
				// Although the message is logged in this case, it's not necessarily an indication
				// that anything is wrong. For example, if a call to RETURN_IF_FAILED() fails, this
				// function will be called, even though the failure might be legitimate and fully
				// expected to happen in some situations.
				LOG(INFO) << logMessageUtf8;
			}
		});

	return glogCleanup;
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
