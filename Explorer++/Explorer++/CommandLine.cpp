// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "CommandLine.h"
#include "CrashHandlerHelper.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/SetDefaultFileManager.h"
#include "../Helper/WindowHelper.h"
#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>
#include <iostream>
#include <optional>

// Allows std::wstring to be used as a custom type for add_option.
// See https://github.com/CLIUtils/CLI11/issues/704#issuecomment-1047954575.
namespace CLI::detail
{
template <>
struct classify_object<std::wstring, void>
{
	static constexpr object_category value{ object_category::other };
};

template <>
struct is_mutable_container<std::wstring, void> : public std::false_type
{
};

template <>
bool lexical_cast(const std::string &input, std::wstring &output)
{
	output = utf8StrToWstr(input);
	return true;
}
}

using CrashedDataTuple = std::tuple<DWORD, DWORD, intptr_t, std::string>;

using namespace DefaultFileManager;

// The items here are handled immediately and don't need to be passed to the Explorerplusplus class.
struct ImmediatelyHandledOptions
{
	bool clearRegistrySettings;
	bool removeAsDefault;
	ReplaceExplorerMode replaceExplorerMode;
	bool jumplistNewTab;
	CrashedDataTuple crashedDataTuple;
};

struct ReplaceExplorerResults
{
	std::optional<LSTATUS> removedFileSystem;
	std::optional<LSTATUS> removedAll;
	std::optional<LSTATUS> setFileSystem;
	std::optional<LSTATUS> setAll;
};

void PreprocessDirectories(std::vector<std::wstring> &directories);
std::optional<CommandLine::ExitInfo> ProcessCommandLineFlags(const CLI::App &app,
	const ImmediatelyHandledOptions &immediatelyHandledOptions, CommandLine::Settings &settings);
void OnClearRegistrySettings();
void OnUpdateReplaceExplorerSetting(ReplaceExplorerMode updatedReplaceMode);
ReplaceExplorerResults UpdateReplaceExplorerSetting(ReplaceExplorerMode updatedReplaceMode);
std::optional<CommandLine::ExitInfo> OnJumplistNewTab();

std::variant<CommandLine::Settings, CommandLine::ExitInfo> CommandLine::ProcessCommandLine()
{
	CLI::App app("Explorer++");
	app.allow_extras();

	ImmediatelyHandledOptions immediatelyHandledOptions;

	immediatelyHandledOptions.clearRegistrySettings = false;
	app.add_flag("--clear-registry-settings", immediatelyHandledOptions.clearRegistrySettings,
		"Clear existing registry settings");

	immediatelyHandledOptions.removeAsDefault = false;
	auto removeAsDefaultOption = app.add_flag("--remove-as-default",
		immediatelyHandledOptions.removeAsDefault, "Remove Explorer++ as the default file manager");

	immediatelyHandledOptions.replaceExplorerMode = ReplaceExplorerMode::None;
	auto setAsDefaultOption =
		app.add_option("--set-as-default", immediatelyHandledOptions.replaceExplorerMode,
			   "Set Explorer++ as the default file manager for the current user")
			->transform(CLI::CheckedTransformer(CLI::TransformPairs<ReplaceExplorerMode>{
				{ "filesystem", ReplaceExplorerMode::FileSystem },
				{ "all", ReplaceExplorerMode::All } }));

	removeAsDefaultOption->excludes(setAsDefaultOption);
	setAsDefaultOption->excludes(removeAsDefaultOption);

	// The options in this group are only used internally by the application. They're not directly
	// exposed to users.
	CLI::App *privateCommands = app.add_subcommand();
	privateCommands->group("");

	immediatelyHandledOptions.jumplistNewTab = false;
	privateCommands->add_flag(wstrToUtf8Str(JUMPLIST_TASK_NEWTAB_ARGUMENT),
		immediatelyHandledOptions.jumplistNewTab);

	privateCommands->add_option(wstrToUtf8Str(APPLICATION_CRASHED_ARGUMENT),
		immediatelyHandledOptions.crashedDataTuple);

	CommandLine::Settings settings;

	std::map<std::string, Feature> featureMap;

	for (auto item : Feature::_values())
	{
		featureMap.insert({ item._to_string(), item });
	}

	app.add_flag("--enable-logging", settings.enableLogging, "Enable logging");

	app.add_option("--enable-features", settings.enableFeatures,
		   "Allows incomplete features that are disabled by default to be enabled")
		->transform(CLI::CheckedTransformer(featureMap));

	app.add_option("--shell-change-notification-type", settings.shellChangeNotificationType,
		   "Watch for directory changes through SHChangeNotifyRegister")
		->transform(CLI::CheckedTransformer(CLI::TransformPairs<ShellChangeNotificationType>{
			{ "disabled", ShellChangeNotificationType::Disabled },
			{ "non-filesystem", ShellChangeNotificationType::NonFilesystem },
			{ "all", ShellChangeNotificationType::All } }));

	app.add_option("--language", settings.language,
		"Allows you to select your desired language. Should be a two-letter language code (e.g. "
		"FR, RU, etc).");

	// Note that allow_extra_args is set to false, which means that multiple items need to be
	// specified by supplying the option multiple times. That's done because allowing multiple items
	// to be specified at once would create ambiguity with the directories option:
	//
	// explorer++.exe --select c:\windows c:\users\public
	//
	// That could mean: select two items (c:\windows and c:\users\public) or select one item
	// (c:\windows) and open a directory (c:\users\public).
	app.add_option("--select", settings.filesToSelect,
		   "When supplied a path like \"C:\\path\\to\\file\", will open \"C:\\path\\to\" in a tab "
		   "and select \"file\". This option can be supplied multiple times, in which case each "
		   "path will be opened in a separate tab.")
		->allow_extra_args(false);

	app.add_option("directories", settings.directories, "Directories to open");

	int numArgs;
	LPWSTR *args = CommandLineToArgvW(GetCommandLine(), &numArgs);

	if (!args)
	{
		return settings;
	}

	auto freeArgs = wil::scope_exit([args] { LocalFree(args); });

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
	catch (const CLI::ParseError &e)
	{
		return ExitInfo{ app.exit(e) };
	}

	auto exitInfo = ProcessCommandLineFlags(app, immediatelyHandledOptions, settings);

	if (exitInfo)
	{
		return *exitInfo;
	}

	PreprocessDirectories(settings.directories);

	return settings;
}

void PreprocessDirectories(std::vector<std::wstring> &directories)
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
	for (std::wstring &directory : directories)
	{
		if (directory[directory.size() - 1] == '\"')
		{
			directory[directory.size() - 1] = '\\';
		}
	}
}

std::optional<CommandLine::ExitInfo> ProcessCommandLineFlags(const CLI::App &app,
	const ImmediatelyHandledOptions &immediatelyHandledOptions, CommandLine::Settings &settings)
{
	if (app.count(wstrToUtf8Str(CommandLine::APPLICATION_CRASHED_ARGUMENT)) > 0)
	{
		auto &crashedDataTuple = immediatelyHandledOptions.crashedDataTuple;
		HandleProcessCrashedNotification(
			{ std::get<0>(crashedDataTuple), std::get<1>(crashedDataTuple),
				std::get<2>(crashedDataTuple), std::get<3>(crashedDataTuple) });
		return CommandLine::ExitInfo{ EXIT_CODE_NORMAL_CRASH_HANDLER };
	}

	if (immediatelyHandledOptions.jumplistNewTab)
	{
		auto exitInfo = OnJumplistNewTab();

		if (exitInfo)
		{
			return exitInfo;
		}

		settings.createJumplistTab = true;
	}

	if (immediatelyHandledOptions.clearRegistrySettings)
	{
		OnClearRegistrySettings();
	}

	if (immediatelyHandledOptions.removeAsDefault)
	{
		OnUpdateReplaceExplorerSetting(ReplaceExplorerMode::None);
	}
	else if (immediatelyHandledOptions.replaceExplorerMode != +ReplaceExplorerMode::None)
	{
		OnUpdateReplaceExplorerSetting(immediatelyHandledOptions.replaceExplorerMode);
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

std::optional<CommandLine::ExitInfo> OnJumplistNewTab()
{
	wil::unique_mutex_nothrow mutex;

	if (!mutex.try_open(NExplorerplusplus::APPLICATION_MUTEX_NAME))
	{
		return std::nullopt;
	}

	HWND existingWindow = FindWindow(NExplorerplusplus::CLASS_NAME, nullptr);

	if (!existingWindow)
	{
		return std::nullopt;
	}

	COPYDATASTRUCT cds;
	cds.cbData = 0;
	cds.lpData = nullptr;
	SendMessage(existingWindow, WM_COPYDATA, NULL, reinterpret_cast<LPARAM>(&cds));

	BringWindowToForeground(existingWindow);

	return CommandLine::ExitInfo{ EXIT_CODE_NORMAL_EXISTING_PROCESS };
}
