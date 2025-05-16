// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "App.h"
#include "CommandLine.h"
#include "CrashHandlerHelper.h"
#include "StartupCommandLineProcessor.h"
#include "../Helper/SystemClipboardStore.h"
#include <boost/locale.hpp>
#include <wil/result.h>
#include <cstdlib>
#include <format>

[[nodiscard]] unique_glog_shutdown_call InitializeLogging();
void InitializeLocale();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

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

	auto clipboardStore = std::make_unique<SystemClipboardStore>();
	auto exitCode =
		StartupCommandLineProcessor::Process(&commandLineSettings, clipboardStore.get());
	clipboardStore.reset();

	if (exitCode)
	{
		return *exitCode;
	}

	App app(&commandLineSettings);
	return app.Run();
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
