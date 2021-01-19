// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "CommandLine.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "Version.h"
#include "../Helper/Macros.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/SetDefaultFileManager.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/StringHelper.h"
#include "../ThirdParty/CLI11/CLI11.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/format.hpp>
#include <boost/log/core.hpp>
#include <filesystem>
#include <iostream>
#include <map>

using CrashedDataTuple = std::tuple<DWORD, DWORD, intptr_t, std::string>;

struct CrashedData
{
	DWORD processId;
	DWORD threadId;
	intptr_t exceptionPointersAddress;
	std::string eventName;

	CrashedData(const CrashedDataTuple &crashedData)
	{
		processId = std::get<0>(crashedData);
		threadId = std::get<1>(crashedData);
		exceptionPointersAddress = std::get<2>(crashedData);
		eventName = std::get<3>(crashedData);
	}
};

using namespace DefaultFileManager;

extern std::vector<std::wstring> g_commandLineDirectories;
extern bool g_enableDarkMode;

typedef BOOL(WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcee, DWORD ProcessId, HANDLE hFile,
	MINIDUMP_TYPE DumpType, PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

struct CommandLineSettings
{
	bool clearRegistrySettings;
	bool enableLogging;
	bool enablePlugins;
	bool registerForShellNotifications;
	bool removeAsDefault;
	ReplaceExplorerMode replaceExplorerMode;
	std::string language;
	bool jumplistNewTab;
	CrashedDataTuple crashedData;
	std::vector<std::string> directories;
};

struct ReplaceExplorerResults
{
	std::optional<LSTATUS> removedFileSystem;
	std::optional<LSTATUS> removedAll;
	std::optional<LSTATUS> setFileSystem;
	std::optional<LSTATUS> setAll;
};

const int REPORT_ISSUE_BUTTON_ID = 100;
const TCHAR REPORT_ISSUE_URL[] = L"https://github.com/derceg/explorerplusplus/issues/new?labels=bug,crash&template=bug_report.md";

void PreprocessCommandLineSettings(CommandLineSettings &commandLineSettings);
std::optional<CommandLine::ExitInfo> ProcessCommandLineSettings(
	const CLI::App &app, const CommandLineSettings &commandLineSettings);
void OnClearRegistrySettings();
void OnUpdateReplaceExplorerSetting(ReplaceExplorerMode updatedReplaceMode);
ReplaceExplorerResults UpdateReplaceExplorerSetting(ReplaceExplorerMode updatedReplaceMode);
void OnShowCrashedMessage(const CrashedData &crashedData);
std::optional<std::wstring> CreateMiniDump(const CrashedData &crashedData);
HRESULT CALLBACK CrashedDialogCallback(
	HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LONG_PTR data);
void OnJumplistNewTab();

std::optional<CommandLine::ExitInfo> CommandLine::ProcessCommandLine()
{
	CLI::App app("Explorer++");
	app.allow_extras();

	CommandLineSettings commandLineSettings;

	commandLineSettings.clearRegistrySettings = false;
	app.add_flag(
		"--clear-registry-settings",
		commandLineSettings.clearRegistrySettings,
		"Clear existing registry settings"
	);

	commandLineSettings.enableLogging = false;
	app.add_flag(
		"--enable-logging",
		commandLineSettings.enableLogging,
		"Enable logging"
	);

	commandLineSettings.enablePlugins = false;
	app.add_flag(
		"--enable-plugins",
		commandLineSettings.enablePlugins,
		"Enable the Lua plugin system"
	);

	commandLineSettings.registerForShellNotifications = false;
	app.add_flag(
		"--register-for-shell-notifications",
		commandLineSettings.registerForShellNotifications,
		"Watch for directory changes through SHChangeNotifyRegister"
	);

	commandLineSettings.removeAsDefault = false;
	auto removeAsDefaultOption = app.add_flag(
		"--remove-as-default",
		commandLineSettings.removeAsDefault,
		"Remove Explorer++ as the default file manager"
	);

	commandLineSettings.replaceExplorerMode = ReplaceExplorerMode::None;
	auto setAsDefaultOption = app.add_option(
		"--set-as-default",
		commandLineSettings.replaceExplorerMode,
		"Set Explorer++ as the default file manager for the current user"
	)->transform(CLI::CheckedTransformer(CLI::TransformPairs<ReplaceExplorerMode>{
		{ "filesystem", ReplaceExplorerMode::FileSystem },
		{ "all", ReplaceExplorerMode::All }
	}));

	removeAsDefaultOption->excludes(setAsDefaultOption);
	setAsDefaultOption->excludes(removeAsDefaultOption);

	app.add_option(
		"--language",
		commandLineSettings.language,
		"Allows you to select your desired language. Should be a two-letter language code (e.g. FR, RU, etc)."
	);

	app.add_option(
		"directories",
		commandLineSettings.directories,
		"Directories to open"
	);

	// The options in this group are only used internally by the application. They're not directly
	// exposed to users.
	CLI::App *privateCommands = app.add_subcommand();
	privateCommands->group("");

	commandLineSettings.jumplistNewTab = false;
	privateCommands->add_flag(
		wstrToUtf8Str(NExplorerplusplus::JUMPLIST_TASK_NEWTAB_ARGUMENT),
		commandLineSettings.jumplistNewTab
	);

	privateCommands->add_option(
		wstrToUtf8Str(NExplorerplusplus::APPLICATION_CRASHED_ARGUMENT),
		commandLineSettings.crashedData
	);

	int numArgs;
	LPWSTR *args = CommandLineToArgvW(GetCommandLine(), &numArgs);

	if (!args)
	{
		return std::nullopt;
	}

	auto freeArgs = wil::scope_exit([args] {
		LocalFree(args);
	});

	std::vector<std::string> utf8Args;

	for (int i = numArgs - 1; i > 0; i--)
	{
		// The args here are converted from utf-16 to utf-8. While it wouldn't be safe to pass the
		// resulting utf-8 strings to Windows API functions, it should be ok to use them as
		// intermediates (to pass to CLI11).
		utf8Args.emplace_back(wstrToUtf8Str(args[i]));
	}

	try
	{
		app.parse(utf8Args);
	}
	catch (const CLI::ParseError & e)
	{
		return ExitInfo{ app.exit(e) };
	}

	PreprocessCommandLineSettings(commandLineSettings);

	return ProcessCommandLineSettings(app, commandLineSettings);
}

void PreprocessCommandLineSettings(CommandLineSettings &commandLineSettings)
{
	// When Explorer++ is set as the default file manager, it's invoked in the following way when a
	// directory is opened:
	//
	// C:\path\to\Explorer++.exe "[directory_path]"
	//
	// If directory_path is something like C:\, this will result in the following invocation:
	//
	// C:\path\to\Explorer++.exe "C:\"
	//
	// This path argument is then turned into the following string:
	//
	// C:"
	//
	// This is due to the C++ command line parsing rules, as described at:
	//
	// https://docs.microsoft.com/en-us/cpp/cpp/main-function-command-line-args?view=vs-2019#parsing-c-command-line-arguments
	//
	// That is, \" is interpreted as a literal backslash character.
	//
	// That isn't what's intended when being passed a directory path. To resolve this, if a
	// directory path ends in a double quote character, that character is replaced with a backslash
	// character. This should be safe, as a double quote isn't an allowed file name character, so
	// the presence of the double quote character is either a mistake (in which case, no directory
	// will be opened anyway, so the transformation won't make much of a difference), or it's
	// something that's being interpreted as part of the command line parsing.
	for (std::string &directory : commandLineSettings.directories)
	{
		if (directory[directory.size() - 1] == '\"')
		{
			directory[directory.size() - 1] = '\\';
		}
	}
}

std::optional<CommandLine::ExitInfo> ProcessCommandLineSettings(
	const CLI::App &app, const CommandLineSettings &commandLineSettings)
{
	if (app.count(wstrToUtf8Str(NExplorerplusplus::APPLICATION_CRASHED_ARGUMENT)) > 0)
	{
		OnShowCrashedMessage(commandLineSettings.crashedData);
		return CommandLine::ExitInfo{ EXIT_SUCCESS };
	}

	if (commandLineSettings.jumplistNewTab)
	{
		OnJumplistNewTab();
		return CommandLine::ExitInfo{ EXIT_SUCCESS };
	}

	if (commandLineSettings.clearRegistrySettings)
	{
		OnClearRegistrySettings();
	}

	if (commandLineSettings.enableLogging)
	{
		boost::log::core::get()->set_logging_enabled(true);
	}

	if (commandLineSettings.enablePlugins)
	{
		g_enablePlugins = true;
	}

	if (commandLineSettings.registerForShellNotifications)
	{
		g_registerForShellNotifications = true;
	}

	if (commandLineSettings.removeAsDefault)
	{
		OnUpdateReplaceExplorerSetting(ReplaceExplorerMode::None);
	}
	else if (commandLineSettings.replaceExplorerMode != ReplaceExplorerMode::None)
	{
		OnUpdateReplaceExplorerSetting(commandLineSettings.replaceExplorerMode);
	}

	if (!commandLineSettings.language.empty())
	{
		g_bForceLanguageLoad = TRUE;

		StringCchCopy(g_szLang, SIZEOF_ARRAY(g_szLang), utf8StrToWstr(commandLineSettings.language).c_str());
	}

	TCHAR processImageName[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), processImageName, SIZEOF_ARRAY(processImageName));

	std::filesystem::path processDirectoryPath(processImageName);
	processDirectoryPath.remove_filename();

	for (const std::string& directory : commandLineSettings.directories)
	{
		TCHAR szParsingPath[MAX_PATH];
		DecodePath(utf8StrToWstr(directory).c_str(), processDirectoryPath.wstring().c_str(), szParsingPath, SIZEOF_ARRAY(szParsingPath));

		g_commandLineDirectories.emplace_back(szParsingPath);
	}

	return std::nullopt;
}

void OnClearRegistrySettings()
{
	LSTATUS lStatus;

	lStatus = SHDeleteKey(HKEY_CURRENT_USER, NExplorerplusplus::REG_MAIN_KEY);

	if (lStatus == ERROR_SUCCESS)
	{
		std::wcout << L"Settings cleared successfully." << std::endl;
	}
	else
	{
		std::wcerr << L"Settings could not be cleared.\n" << std::endl;
	}
}

void OnUpdateReplaceExplorerSetting(ReplaceExplorerMode updatedReplaceMode)
{
	auto results = UpdateReplaceExplorerSetting(updatedReplaceMode);

	switch (updatedReplaceMode)
	{
	case DefaultFileManager::ReplaceExplorerMode::None:
		if ((results.removedFileSystem && *results.removedFileSystem == ERROR_SUCCESS)
			|| (results.removedAll && *results.removedAll == ERROR_SUCCESS))
		{
			std::wcout << L"Explorer++ successfully removed as default file manager.\n"
					   << std::endl;
		}
		else
		{
			std::wcerr << L"Could not remove Explorer++ as default file manager." << std::endl;
		}
		break;

	case DefaultFileManager::ReplaceExplorerMode::FileSystem:
	case DefaultFileManager::ReplaceExplorerMode::All:
		if ((results.setFileSystem && *results.setFileSystem == ERROR_SUCCESS)
			|| (results.setAll && *results.setAll == ERROR_SUCCESS))
		{
			std::wcout << L"Explorer++ successfully set as default file manager." << std::endl;
		}
		else
		{
			std::wcerr << L"Could not set Explorer++ as default file manager." << std::endl;
		}
		break;
	}
}

ReplaceExplorerResults UpdateReplaceExplorerSetting(ReplaceExplorerMode updatedReplaceMode)
{
	ReplaceExplorerResults results;

	// TODO: This text should be retrieved from the appropriate translation DLL (if necessary).
	std::wstring menuText =
		ResourceHelper::LoadString(GetModuleHandle(nullptr), IDS_OPEN_IN_EXPLORERPLUSPLUS);

	// Whether Explorer++ is being set as the default file manager, or being removed, the first step
	// is always to remove any existing entries.
	results.removedFileSystem =
		RemoveAsDefaultFileManagerFileSystem(SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
	results.removedAll = RemoveAsDefaultFileManagerAll(SHELL_DEFAULT_INTERNAL_COMMAND_NAME);

	switch (updatedReplaceMode)
	{
	case ReplaceExplorerMode::None:
		// This case is effectively handled above.
		break;

	case ReplaceExplorerMode::FileSystem:
		results.setFileSystem = SetAsDefaultFileManagerFileSystem(
			SHELL_DEFAULT_INTERNAL_COMMAND_NAME, menuText.c_str());
		break;

	case ReplaceExplorerMode::All:
		results.setAll =
			SetAsDefaultFileManagerAll(SHELL_DEFAULT_INTERNAL_COMMAND_NAME, menuText.c_str());
		break;
	}

	return results;
}

void OnShowCrashedMessage(const CrashedData &crashedData)
{
	auto crashDumpFileName = CreateMiniDump(crashedData);
	std::wstring message;

	if (crashDumpFileName)
	{
		message = (boost::wformat(L" A crash dump has been saved to:\n\n%s\n\n"
								  L"If you report this crash, please include this crash dump.")
			% *crashDumpFileName)
					  .str();
	}
	else
	{
		message = L"A crash dump could not be created.";
	}

	TASKDIALOG_BUTTON customButtons[] = { { REPORT_ISSUE_BUTTON_ID, L"Report issue..." } };

	int button;

	TASKDIALOGCONFIG dialogConfig = { 0 };
	dialogConfig.cbSize = sizeof(dialogConfig);
	dialogConfig.hwndParent = nullptr;
	dialogConfig.hInstance = GetModuleHandle(nullptr);
	dialogConfig.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_SIZE_TO_CONTENT;
	dialogConfig.dwCommonButtons = TDCBF_CLOSE_BUTTON;
	dialogConfig.pszWindowTitle = NExplorerplusplus::APP_NAME;
	dialogConfig.pszMainIcon = TD_ERROR_ICON;
	dialogConfig.pszMainInstruction = L"Explorer++ has encountered an error.";
	dialogConfig.pszContent = message.c_str();
	dialogConfig.cButtons = SIZEOF_ARRAY(customButtons);
	dialogConfig.pButtons = customButtons;
	dialogConfig.nDefaultButton = IDCLOSE;
	dialogConfig.cRadioButtons = 0;
	dialogConfig.pRadioButtons = nullptr;
	dialogConfig.pszVerificationText = nullptr;
	dialogConfig.pszExpandedInformation = nullptr;
	dialogConfig.pszFooter = nullptr;
	dialogConfig.pfCallback = CrashedDialogCallback;
	dialogConfig.lpCallbackData = 0;
	dialogConfig.cxWidth = 0;
	TaskDialogIndirect(&dialogConfig, &button, nullptr, nullptr);
}

std::optional<std::wstring> CreateMiniDump(const CrashedData &crashedData)
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

	MINIDUMPWRITEDUMP miniDumpWriteDump =
		reinterpret_cast<MINIDUMPWRITEDUMP>(GetProcAddress(dbgHelp.get(), "MiniDumpWriteDump"));

	if (!miniDumpWriteDump)
	{
		return std::nullopt;
	}

	TCHAR fullPath[MAX_PATH];
	DWORD pathRes = GetTempPath(SIZEOF_ARRAY(fullPath), fullPath);

	if (pathRes == 0)
	{
		return std::nullopt;
	}

	SYSTEMTIME localTime;
	GetLocalTime(&localTime);

	TCHAR fileName[MAX_PATH];
	HRESULT hr =
		StringCchPrintf(fileName, SIZEOF_ARRAY(fileName), _T("%s%s-%02d%02d%04d-%02d%02d%02d.dmp"),
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

	wil::unique_hfile file(CreateFile(
		fullPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));

	if (!file)
	{
		return std::nullopt;
	}

	MINIDUMP_EXCEPTION_INFORMATION mei;
	mei.ThreadId = crashedData.threadId;
	mei.ExceptionPointers = exceptionAddress;
	mei.ClientPointers = true;
	res = miniDumpWriteDump(
		process.get(), crashedData.processId, file.get(), MiniDumpNormal, &mei, nullptr, nullptr);

	if (!res)
	{
		return std::nullopt;
	}

	return fullPath;
}

HRESULT CALLBACK CrashedDialogCallback(
	HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LONG_PTR data)
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(lParam);
	UNREFERENCED_PARAMETER(data);

	switch (msg)
	{
	case TDN_BUTTON_CLICKED:
		switch (wParam)
		{
		case REPORT_ISSUE_BUTTON_ID:
			ShellExecute(nullptr, L"open", REPORT_ISSUE_URL, nullptr, nullptr, SW_SHOW);
			return S_FALSE;
		}
		break;
	}

	return S_OK;
}

void OnJumplistNewTab()
{
	/* This will be called when the user clicks the
	'New Tab' item on the tasks menu in Windows 7 and above.
	Find the already opened version of Explorer++,
	and tell it to open a new tab. */
	HANDLE hMutex;

	hMutex = CreateMutex(nullptr, TRUE, _T("Explorer++"));

	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		HWND hPrev;

		hPrev = FindWindow(NExplorerplusplus::CLASS_NAME, nullptr);

		if (hPrev != nullptr)
		{
			COPYDATASTRUCT cds;

			cds.cbData = 0;
			cds.lpData = nullptr;
			SendMessage(hPrev, WM_COPYDATA, NULL, (LPARAM)& cds);

			SetForegroundWindow(hPrev);
			ShowWindow(hPrev, SW_SHOW);
		}
	}

	if (hMutex != nullptr)
	{
		CloseHandle(hMutex);
	}
}