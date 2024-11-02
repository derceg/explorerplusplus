// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AcceleratorManager.h"
#include "App.h"
#include "CommandLine.h"
#include "CrashHandlerHelper.h"
#include "ExitCode.h"
#include "Explorer++_internal.h"
#include "RegistrySettings.h"
#include "StartupCommandLineProcessor.h"
#include "XMLSettings.h"
#include "../Helper/WindowHelper.h"
#include <boost/locale.hpp>
#include <wil/resource.h>
#include <cstdlib>
#include <format>

struct WindowState
{
	RECT bounds;
	int showState;
};

[[nodiscard]] unique_glog_shutdown_call InitializeLogging();
void InitializeLocale();
RECT GetDefaultMainWindowBounds();
std::optional<WindowState> LoadMainWindowState(bool loadSettingsFromXML);
bool IsModelessDialogMessage(App *app, MSG *msg);
bool MaybeTranslateAccelerator(App *app, MSG *msg);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// It's important that this is what's done first for two reasons:
	//
	// 1. By default, logging is disabled. What that means, specifically, is that logging error
	// statements will go to stdout by default. That's pointless unless there's a console attached.
	// Doing this first means that early CHECK failures (ones that occur before file logging has the
	// chance to be set up, which occurs after the command line has been parsed) can still be
	// potentially shown somewhere. It also means that this is the only point in the application
	// where log statements can be silently discarded.
	//
	// 2. As part of parsing the command line, text (e.g. help text) can be printed to the console.
	// As with the above, that's not useful unless there's a console attached.
	auto consoleCleanup = AttachParentConsole();

	// Logging and crash handling are both explicitly initialized early, so that they're available
	// for almost the entire lifetime of the application (the only exception being the above block).
	auto glogCleanup = InitializeLogging();
	InitializeCrashHandler();
	InitializeLocale();

	auto commandLineInfo = CommandLine::Parse(GetCommandLine());

	if (std::holds_alternative<CommandLine::ExitInfo>(commandLineInfo))
	{
		return std::get<CommandLine::ExitInfo>(commandLineInfo).exitCode;
	}

	const auto &commandLineSettings = std::get<CommandLine::Settings>(commandLineInfo);

	auto exitCode = StartupCommandLineProcessor::Process(&commandLineSettings);

	if (exitCode)
	{
		return *exitCode;
	}

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

	App app(&commandLineSettings);

	WindowState windowState(GetDefaultMainWindowBounds(), nCmdShow);
	auto loadedWindowState = LoadMainWindowState(bLoadSettingsFromXML);

	if (loadedWindowState)
	{
		windowState = *loadedWindowState;
	}

	Explorerplusplus::Create(&app, &windowState.bounds, windowState.showState);

	MSG msg;

	while (GetMessage(&msg, nullptr, 0, 0) > 0)
	{
		if (!IsModelessDialogMessage(&app, &msg) && !MaybeTranslateAccelerator(&app, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return static_cast<int>(msg.wParam);
}

unique_glog_shutdown_call InitializeLogging()
{
	// By default, logging will be disabled. That is, logs will only go to stdout, which effectively
	// disables them from the user's perspective.
	FLAGS_logtostdout = true;

	// Doing this means that, if the application was launched from the console, for example, CHECK
	// failures will still be shown, which is somewhat useful.
	FLAGS_minloglevel = google::GLOG_ERROR;

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

RECT GetDefaultMainWindowBounds()
{
	RECT workArea;
	BOOL res = SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
	CHECK(res);

	// The strategy here is fairly simple - the window will be sized to a portion of the work area
	// of the primary monitor and centered.
	auto width = static_cast<int>(GetRectWidth(&workArea) * 0.60);
	auto height = static_cast<int>(GetRectHeight(&workArea) * 0.60);
	int x = (GetRectWidth(&workArea) - width) / 2;
	int y = (GetRectHeight(&workArea) - height) / 2;

	return { x, y, x + width, y + height };
}

std::optional<WindowState> LoadMainWindowState(bool loadSettingsFromXML)
{
	WINDOWPLACEMENT placement;
	BOOL loaded;

	if (loadSettingsFromXML)
	{
		loaded = LoadWindowPositionFromXML(&placement);
	}
	else
	{
		loaded = LoadWindowPositionFromRegistry(&placement);
	}

	if (!loaded)
	{
		return std::nullopt;
	}

	// When shown in its normal size, the window for the application should at least be on screen
	// somewhere, even if it's not completely visible. Therefore, the position should be reset if
	// the window won't be visible on any monitor.
	// Checking this on startup makes sense, since the monitor setup can change in between
	// executions.
	HMONITOR monitor = MonitorFromRect(&placement.rcNormalPosition, MONITOR_DEFAULTTONULL);

	if (!monitor)
	{
		return std::nullopt;
	}

	if (placement.showCmd != SW_SHOWNORMAL && placement.showCmd != SW_SHOWMAXIMIZED)
	{
		placement.showCmd = SW_SHOWNORMAL;
	}

	return WindowState(placement.rcNormalPosition, placement.showCmd);
}

bool IsModelessDialogMessage(App *app, MSG *msg)
{
	for (auto modelessDialog : app->GetModelessDialogList()->GetList())
	{
		if (IsChild(modelessDialog, msg->hwnd))
		{
			return IsDialogMessage(modelessDialog, msg);
		}
	}

	return false;
}

bool MaybeTranslateAccelerator(App *app, MSG *msg)
{
	for (auto *browser : app->GetBrowserList()->GetList())
	{
		if (IsChild(browser->GetHWND(), msg->hwnd))
		{
			return TranslateAccelerator(browser->GetHWND(),
				app->GetAcceleratorManager()->GetAcceleratorTable(), msg);
		}
	}

	return false;
}
