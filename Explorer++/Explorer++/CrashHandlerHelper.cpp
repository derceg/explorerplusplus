// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "CrashHandlerHelper.h"
#include "App.h"
#include "ApplicationCrashedDialog.h"
#include "CommandLine.h"
#include "Version.h"
#include "VersionHelper.h"
#include "../Helper/DetoursHelper.h"
#include "../Helper/Helper.h"
#include "../Helper/ProcessHelper.h"
#include <boost/algorithm/string/join.hpp>
#include <boost/pfr.hpp>
#include <detours/detours.h>
#include <glog/logging.h>
#include <wil/resource.h>
#include <DbgHelp.h>
#include <format>

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
		DCHECK(false);
		LOG(WARNING) << "Error when attempting to disable SetUnhandledExceptionFilter: " << res;
	}
}

LONG WINAPI TopLevelExceptionFilter(EXCEPTION_POINTERS *exception)
{
	TCHAR currentProcess[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), currentProcess, std::size(currentProcess));

	// Event names are global in the system. Therefore, the event name used for signaling should be
	// unique.
	auto eventName = CreateGUID();
	wil::unique_event_nothrow event;
	bool res = event.try_create(wil::EventOptions::ManualReset, eventName.c_str());

	if (!res)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	CrashedData crashedData;
	crashedData.processId = GetCurrentProcessId();
	crashedData.threadId = GetCurrentThreadId();
	crashedData.exceptionPointersAddress = std::bit_cast<intptr_t>(exception);
	crashedData.eventName = eventName;

	std::wstring arguments = std::format(L"\"{}\" {} {}", currentProcess,
		CommandLine::APPLICATION_CRASHED_ARGUMENT, FormatCrashedDataForCommandLine(crashedData));

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

std::wstring FormatCrashedDataForCommandLine(const CrashedData &crashedData)
{
	std::vector<std::wstring> fields;
	boost::pfr::for_each_field(crashedData,
		[&fields](const auto &field) { fields.push_back(std::format(L"{}", field)); });
	return boost::algorithm::join(fields, L" ");
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
	bool res = event.try_open(crashedData.eventName.c_str());

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

	TCHAR fullPath[MAX_PATH];
	DWORD pathRes = GetTempPath(static_cast<DWORD>(std::size(fullPath)), fullPath);

	if (pathRes == 0)
	{
		return std::nullopt;
	}

	SYSTEMTIME localTime;
	GetLocalTime(&localTime);

	TCHAR fileName[MAX_PATH];
	HRESULT hr = StringCchPrintf(fileName, std::size(fileName),
		_T("%s%s-%02d%02d%04d-%02d%02d%02d.dmp"), App::APP_NAME,
		VersionHelper::GetVersion().GetString().c_str(), localTime.wDay, localTime.wMonth,
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
	mei.ExceptionPointers =
		std::bit_cast<EXCEPTION_POINTERS *>(crashedData.exceptionPointersAddress);
	mei.ClientPointers = true;
	res = MiniDumpWriteDump(process.get(), crashedData.processId, file.get(), MiniDumpNormal, &mei,
		nullptr, nullptr);

	if (!res)
	{
		return std::nullopt;
	}

	return fullPath;
}
