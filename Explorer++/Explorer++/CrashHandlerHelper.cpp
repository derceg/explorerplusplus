// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "CrashHandlerHelper.h"
#include "Explorer++_internal.h"
#include "../Detours/detours.h"
#include "../Helper/Helper.h"
#include "../Helper/Logging.h"
#include "../Helper/ProcessHelper.h"
#include <wil/resource.h>

LONG WINAPI TopLevelExceptionFilter(EXCEPTION_POINTERS *exception);
LONG DisableSetUnhandledExceptionFilter();
LPTOP_LEVEL_EXCEPTION_FILTER WINAPI DetouredSetUnhandledExceptionFilter(
	LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter);

auto OriginalSetUnhandledExceptionFilter = SetUnhandledExceptionFilter;

void InitializeCrashHandler()
{
	SetUnhandledExceptionFilter(TopLevelExceptionFilter);

	// Shell extensions will run in-process and can call SetUnhandledExceptionFilter() at any time.
	// For example, the Google Drive shell extension will call SetUnhandledExceptionFilter() the
	// first time an icon is retrieved (using SHGetFileInfo()).
	// As the filter function is being used to implement crash handling, it's important that it not
	// be removed. Therefore, the function will be detoured to a dummy version that doesn't actually
	// do anything.
	LONG res = DisableSetUnhandledExceptionFilter();

	if (res != NO_ERROR)
	{
		assert(false);
		LOG(warning) << L"Error when attempting to disable SetUnhandledExceptionFilter: " << res;
	}
}

LONG WINAPI TopLevelExceptionFilter(EXCEPTION_POINTERS *exception)
{
	TCHAR currentProcess[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), currentProcess,
		static_cast<DWORD>(std::size(currentProcess)));

	// Event names are global in the system. Therefore, the event name used for signaling should be
	// unique.
	auto eventName = CreateGUID();
	wil::unique_event_nothrow event;
	bool res = event.try_create(wil::EventOptions::ManualReset, eventName.c_str());

	if (!res)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	std::wstring arguments = std::format(L"\"{}\" {} {} {} {} {}", currentProcess,
		NExplorerplusplus::APPLICATION_CRASHED_ARGUMENT, GetCurrentProcessId(),
		GetCurrentThreadId(), static_cast<void *>(exception), eventName);

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

LONG DisableSetUnhandledExceptionFilter()
{
	LONG res = DetourTransactionBegin();

	if (res != NO_ERROR)
	{
		return res;
	}

	res = DetourUpdateThread(GetCurrentThread());

	if (res != NO_ERROR)
	{
		return res;
	}

	res = DetourAttach(&(PVOID &) OriginalSetUnhandledExceptionFilter,
		DetouredSetUnhandledExceptionFilter);

	if (res != NO_ERROR)
	{
		return res;
	}

	res = DetourTransactionCommit();

	if (res != NO_ERROR)
	{
		return res;
	}

	return res;
}

LPTOP_LEVEL_EXCEPTION_FILTER WINAPI DetouredSetUnhandledExceptionFilter(
	LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
{
	UNREFERENCED_PARAMETER(lpTopLevelExceptionFilter);

	// This function intentionally takes no action. Any attempt to set an unhandled exception filter
	// will be explicitly ignored.
	return nullptr;
}
