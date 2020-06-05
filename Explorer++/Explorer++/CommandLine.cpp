// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "CommandLine.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/SetDefaultFileManager.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/StringHelper.h"
#include "../ThirdParty/CLI11/CLI11.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/core.hpp>
#include <iostream>
#include <map>

using namespace DefaultFileManager;

extern std::vector<std::wstring> g_commandLineDirectories;
extern bool g_enableDarkMode;

struct CommandLineSettings
{
	bool clearRegistrySettings;
	bool enableLogging;
	bool enablePlugins;
	bool removeAsDefault;
	ReplaceExplorerMode replaceExplorerMode;
	std::string language;
	bool jumplistNewTab;
	bool enableDarkMode;
	std::vector<std::string> directories;
};

struct ReplaceExplorerResults
{
	std::optional<LSTATUS> removedFileSystem;
	std::optional<LSTATUS> removedAll;
	std::optional<LSTATUS> setFileSystem;
	std::optional<LSTATUS> setAll;
};

void PreprocessCommandLineSettings(CommandLineSettings &commandLineSettings);
std::optional<CommandLine::ExitInfo> ProcessCommandLineSettings(const CommandLineSettings& commandLineSettings);
void OnClearRegistrySettings();
void OnUpdateReplaceExplorerSetting(ReplaceExplorerMode updatedReplaceMode);
ReplaceExplorerResults UpdateReplaceExplorerSetting(ReplaceExplorerMode updatedReplaceMode);
void OnJumplistNewTab();

std::optional<CommandLine::ExitInfo> CommandLine::ProcessCommandLine()
{
	CLI::App app("Explorer++");

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

	commandLineSettings.enableDarkMode = false;
	app.add_flag(
		"--enable-dark-mode",
		commandLineSettings.enableDarkMode,
		"(Experimental) Enables dark mode. Only tested with Windows 10 version 1909. May fail or \
crash with other versions of Windows 10. This option has no effect on earlier versions of Windows."
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
		wstrToStr(NExplorerplusplus::JUMPLIST_TASK_NEWTAB_ARGUMENT),
		commandLineSettings.jumplistNewTab
	);

	try
	{
		app.parse(__argc, __argv);
	}
	catch (const CLI::ParseError & e)
	{
		return ExitInfo{ app.exit(e) };
	}

	PreprocessCommandLineSettings(commandLineSettings);

	return ProcessCommandLineSettings(commandLineSettings);
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

std::optional<CommandLine::ExitInfo> ProcessCommandLineSettings(const CommandLineSettings &commandLineSettings)
{
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

		StringCchCopy(g_szLang, SIZEOF_ARRAY(g_szLang), strToWstr(commandLineSettings.language).c_str());
	}

	g_enableDarkMode = commandLineSettings.enableDarkMode;

	TCHAR processImageName[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), processImageName, SIZEOF_ARRAY(processImageName));

	boost::filesystem::path processDirectoryPath(processImageName);
	processDirectoryPath.remove_filename();

	for (const std::string& directory : commandLineSettings.directories)
	{
		TCHAR szParsingPath[MAX_PATH];
		DecodePath(strToWstr(directory).c_str(), processDirectoryPath.wstring().c_str(), szParsingPath, SIZEOF_ARRAY(szParsingPath));

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