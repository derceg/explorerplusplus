// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "CommandLine.h"
#include "ClipboardOperations.h"
#include "CommandLineSplitter.h"
#include "CrashHandlerHelper.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "PasteSymLinksClient.h"
#include "ResourceHelper.h"
#include "../Helper/SetDefaultFileManager.h"
#include "../Helper/WindowHelper.h"
#include <CLI/CLI.hpp>
#include <boost/pfr.hpp>
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

using namespace DefaultFileManager;

// The items here are handled immediately and don't need to be passed to the Explorerplusplus class.
struct ImmediatelyHandledOptions
{
	bool clearRegistrySettings;
	bool removeAsDefault;
	ReplaceExplorerMode replaceExplorerMode;
	bool jumplistNewTab;
	std::optional<CrashedData> crashedData;
	std::wstring pasteSymLinksDestination;
};

struct ReplaceExplorerResults
{
	std::optional<LSTATUS> removedFileSystem;
	std::optional<LSTATUS> removedAll;
	std::optional<LSTATUS> setFileSystem;
	std::optional<LSTATUS> setAll;
};

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

	// CLI11 can parse a set of different value types for a single option, provided those types are
	// specified by a tuple. That's the reason the appropriate tuple type is derived here. If the
	// application crashed argument is supplied, the tuple will be converted back to a struct by the
	// callback. The callback itself runs as part of the parse() call made below, so capturing local
	// variables here is safe.
	using CrashedDataTuple = decltype(boost::pfr::structure_to_tuple(std::declval<CrashedData>()));
	privateCommands->add_option_function<CrashedDataTuple>(
		wstrToUtf8Str(APPLICATION_CRASHED_ARGUMENT),
		[&immediatelyHandledOptions](const CrashedDataTuple &crashedDataTuple) {
			immediatelyHandledOptions.crashedData =
				std::make_from_tuple<CrashedData>(crashedDataTuple);
		});

	privateCommands->add_option(wstrToUtf8Str(PASTE_SYMLINKS_ARGUMENT),
		immediatelyHandledOptions.pasteSymLinksDestination);

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
		   R"(When supplied a path like "C:\path\to\file", will open "C:\path\to" in a tab and )"
		   R"(select "file". This option can be supplied multiple times, in which case each path )"
		   "will be opened in a separate tab.")
		->allow_extra_args(false);

	app.add_option("directories", settings.directories,
		"Directories to open. Paths with spaces should be enclosed in double quotes (e.g. "
		R"("C:\path with spaces").)");

	auto splitResult = CommandLineSplitter::Split(wstrToUtf8Str(GetCommandLine()));

	if (!splitResult.succeeded)
	{
		std::cerr << splitResult.errorMessage << std::endl;

		return ExitInfo{ EXIT_CODE_ERROR };
	}

	// There should always be at least one argument present (the executable name). That argument
	// shouldn't be passed through to CLI11. As CommandLineSplitter expects that there is always at
	// least one argument, the call above will fail if that's not the case. Which means that this
	// CHECK() should always succeed.
	CHECK(!splitResult.arguments.empty());
	splitResult.arguments.erase(splitResult.arguments.begin());

	// CLI11 requires arguments be provided in reverse order.
	std::reverse(splitResult.arguments.begin(), splitResult.arguments.end());

	try
	{
		app.parse(splitResult.arguments);
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

	return settings;
}

std::optional<CommandLine::ExitInfo> ProcessCommandLineFlags(const CLI::App &app,
	const ImmediatelyHandledOptions &immediatelyHandledOptions, CommandLine::Settings &settings)
{
	if (immediatelyHandledOptions.crashedData)
	{
		HandleProcessCrashedNotification(*immediatelyHandledOptions.crashedData);
		return CommandLine::ExitInfo{ EXIT_CODE_NORMAL_CRASH_HANDLER };
	}

	if (app.count(wstrToUtf8Str(CommandLine::PASTE_SYMLINKS_ARGUMENT)) > 0)
	{
		auto pastedItems =
			ClipboardOperations::PasteSymLinks(immediatelyHandledOptions.pasteSymLinksDestination);

		PasteSymLinksClient client;
		client.NotifyServerOfResult(pastedItems);

		return CommandLine::ExitInfo{ EXIT_CODE_NORMAL };
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
