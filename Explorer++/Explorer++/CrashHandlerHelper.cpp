// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "CrashHandlerHelper.h"
#include "ApplicationCrashedDialog.h"
#include "Explorer++_internal.h"
#include "Version.h"
#include "../Helper/DetoursHelper.h"
#include "../Helper/Helper.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/StringHelper.h"
#include <glog/logging.h>
#include <wil/resource.h>
#include <detours/detours.h>
#include <format>

using MiniDumpWriteDumpType = BOOL(WINAPI *)(HANDLE hProcee, DWORD ProcessId, HANDLE hFile,
	MINIDUMP_TYPE DumpType, PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

LONG WINAPI TopLevelExceptionFilter(EXCEPTION_POINTERS *exception);
LONG DisableSetUnhandledExceptionFilter();
LPTOP_LEVEL_EXCEPTION_FILTER WINAPI DetouredSetUnhandledExceptionFilter(
	LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter);

std::optional<std::wstring> CreateMiniDumpForCrashedProcess(const CrashedData &crashedData);

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
		LOG(WARNING) << "Error when attempting to disable SetUnhandledExceptionFilter: " << res;
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

	// The order of the arguments here needs to match the order of the arguments in CommandLine.cpp.
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
	return DetourTransaction(
		[]
		{
			return DetourAttach(&(PVOID &) OriginalSetUnhandledExceptionFilter,
				DetouredSetUnhandledExceptionFilter);
		});
}

LPTOP_LEVEL_EXCEPTION_FILTER WINAPI DetouredSetUnhandledExceptionFilter(
	LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
{
	UNREFERENCED_PARAMETER(lpTopLevelExceptionFilter);

	// This function intentionally takes no action. Any attempt to set an unhandled exception filter
	// will be explicitly ignored.
	return nullptr;
}

// This handles the notification that an Explorer++ process (not this one) has crashed.
void HandleProcessCrashedNotification(const CrashedData &crashedData)
{
	auto crashDumpFileName = CreateMiniDumpForCrashedProcess(crashedData);

	ApplicationCrashedDialog applicationCrashedDialog(crashDumpFileName);
	applicationCrashedDialog.Show();
}

std::optional<std::wstring> CreateMiniDumpForCrashedProcess(const CrashedData &crashedData)
{
	wil::unique_event_nothrow event;
	bool res = event.try_open(utf8StrToWstr(crashedData.eventName).c_str());

	if (!res)
	{
		return std::nullopt;
	}

	// The original process will wait until this event is signaled to exit. It's important that the
	// original process exists until the MiniDumpWriteDump call below finishes.
	// By signaling the event whenever the current function returns, the event will either be
	// signaled when the MiniDumpWriteDump call has completed, or when one of the intermediate steps
	// has failed.
	auto setOnExit = event.SetEvent_scope_exit();

	wil::unique_process_handle process(
		OpenProcess(PROCESS_ALL_ACCESS, false, crashedData.processId));

	if (!process)
	{
		return std::nullopt;
	}

	wil::unique_handle thread(OpenThread(THREAD_ALL_ACCESS, false, crashedData.threadId));

	if (!thread)
	{
		return std::nullopt;
	}

	EXCEPTION_POINTERS *exceptionAddress =
		reinterpret_cast<EXCEPTION_POINTERS *>(crashedData.exceptionPointersAddress);

	wil::unique_hmodule dbgHelp(LoadLibrary(_T("Dbghelp.dll")));

	if (!dbgHelp)
	{
		return std::nullopt;
	}

	auto miniDumpWriteDump =
		reinterpret_cast<MiniDumpWriteDumpType>(GetProcAddress(dbgHelp.get(), "MiniDumpWriteDump"));

	if (!miniDumpWriteDump)
	{
		return std::nullopt;
	}

	TCHAR fullPath[MAX_PATH];
	DWORD pathRes = GetTempPath(static_cast<DWORD>(std::size(fullPath)), fullPath);

	if (pathRes == 0)
	{
		return std::nullopt;
	}

	SYSTEMTIME localTime;
	GetLocalTime(&localTime);

	TCHAR fileName[MAX_PATH];
	HRESULT hr =
		StringCchPrintf(fileName, std::size(fileName), _T("%s%s-%02d%02d%04d-%02d%02d%02d.dmp"),
			NExplorerplusplus::APP_NAME, VERSION_STRING_W, localTime.wDay, localTime.wMonth,
			localTime.wYear, localTime.wHour, localTime.wMinute, localTime.wSecond);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	res = PathAppend(fullPath, fileName);

	if (!res)
	{
		return std::nullopt;
	}

	wil::unique_hfile file(CreateFile(fullPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, nullptr));

	if (!file)
	{
		return std::nullopt;
	}

	MINIDUMP_EXCEPTION_INFORMATION mei;
	mei.ThreadId = crashedData.threadId;
	mei.ExceptionPointers = exceptionAddress;
	mei.ClientPointers = true;
	res = miniDumpWriteDump(process.get(), crashedData.processId, file.get(), MiniDumpNormal, &mei,
		nullptr, nullptr);

	if (!res)
	{
		return std::nullopt;
	}

	return fullPath;
}
