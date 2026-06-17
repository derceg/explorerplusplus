// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "CommandLine.h"
#include "CommandLineSplitter.h"
#include "ExitCode.h"
#include "../Helper/StringHelper.h"
#include "../Helper/ShellHelper.h"
#include <CLI/CLI.hpp>
#include <boost/pfr.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>

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

namespace CommandLine
{

std::variant<Settings, ExitInfo> Parse(const std::wstring &commandLine)
{
	CLI::App app("Explorer++");
	app.allow_extras();

	Settings settings;

	app.add_flag("-c,--clear-settings", settings.clearRegistrySettings,
		"Clear existing registry settings");

	auto removeAsDefaultOption = app.add_flag("-r,--remove-default", settings.removeAsDefault,
		"Remove Explorer++ as the default file manager");

	auto setAsDefaultOption =
		app.add_option("-d,--set-default", settings.replaceExplorerMode,
			   "Set Explorer++ as the default file manager for the current user")
			->transform(CLI::CheckedTransformer(
				CLI::TransformPairs<DefaultFileManager::ReplaceExplorerMode>{
					{ "filesystem", DefaultFileManager::ReplaceExplorerMode::FileSystem },
					{ "all", DefaultFileManager::ReplaceExplorerMode::All } }));

	removeAsDefaultOption->excludes(setAsDefaultOption);
	setAsDefaultOption->excludes(removeAsDefaultOption);

	// The options in this group are only used internally by the application. They're not directly
	// exposed to users.
	CLI::App *privateCommands = app.add_subcommand();
	privateCommands->group("");

	privateCommands->add_flag(wstrToUtf8Str(JUMPLIST_TASK_NEWTAB_ARGUMENT),
		settings.jumplistNewTab);

	// CLI11 can parse a set of different value types for a single option, provided those types are
	// specified by a tuple. That's the reason the appropriate tuple type is derived here. If the
	// application crashed argument is supplied, the tuple will be converted back to a struct by the
	// callback. The callback itself runs as part of the parse() call made below, so capturing local
	// variables here is safe.
	using CrashedDataTuple = decltype(boost::pfr::structure_to_tuple(std::declval<CrashedData>()));
	privateCommands->add_option_function<CrashedDataTuple>(
		wstrToUtf8Str(APPLICATION_CRASHED_ARGUMENT),
		[&settings](const CrashedDataTuple &crashedDataTuple)
		{ settings.crashedData = std::make_from_tuple<CrashedData>(crashedDataTuple); });

	privateCommands->add_option(wstrToUtf8Str(PASTE_SYMLINKS_ARGUMENT),
		settings.pasteSymLinksDestination);

	app.add_flag("-l,--log", settings.enableLogging, "Enable logging");

	std::map<std::string, Feature> featureMap;

	for (auto item : Feature::_values())
	{
		featureMap.insert({ item._to_string(), item });
	}

	app.add_option("-f,--features", settings.featuresToEnable,
		   "Allows incomplete features that are disabled by default to be enabled")
		->transform(CLI::CheckedTransformer(featureMap));

	app.add_option("-n,--notify", settings.changeNotifyMode,
		   "Allows the directory change implementation to be selected")
		->transform(CLI::CheckedTransformer(
			CLI::TransformPairs<ChangeNotifyMode>{ { "shell", ChangeNotifyMode::Shell },
				{ "filesystem", ChangeNotifyMode::Filesystem } }));

	app.add_option("-L,--lang", settings.language,
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
	app.add_option("-s,--select", settings.filesToSelect,
		   R"(When supplied a path like "C:\path\to\file", will open "C:\path\to" in a tab and )"
		   R"(select "file". This option can be supplied multiple times, in which case each path )"
		   "will be opened in a separate tab.")
		->allow_extra_args(false);

	app.add_option("directories", settings.directories,
		"Directories to open. Paths with spaces should be enclosed in double quotes (e.g. "
		R"("C:\path with spaces").)");

	auto splitResult = CommandLineSplitter::Split(wstrToUtf8Str(commandLine));

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

	// Process Windows Explorer style arguments and resolve relative paths.
	auto currentDirectory = GetCurrentDirectoryWrapper();
	std::vector<std::string> finalArguments;

	for (size_t i = 0; i < splitResult.arguments.size(); ++i)
	{
		const std::string &arg = splitResult.arguments[i];
		std::wstring wArg = utf8StrToWstr(arg);
		std::wstring wArgLower = wArg;
		boost::algorithm::to_lower(wArgLower);

		if (wArgLower == L"/n")
		{
			settings.openNewWindow = true;
			continue;
		}
		else if (wArgLower == L"/e")
		{
			// /e is default behavior for Explorer++ (explorer view)
			continue;
		}
		else if (wArgLower.length() >= 8 && wArgLower.substr(0, 8) == L"/select,")
		{
			std::wstring path = wArg.substr(8);
			if (currentDirectory)
			{
				auto absolutePath = TransformUserEnteredPathToAbsolutePathAndNormalize(path,
					currentDirectory.value(), EnvVarsExpansion::DontExpand);
				if (absolutePath)
				{
					settings.filesToSelect.push_back(absolutePath.value());
				}
				else
				{
					settings.filesToSelect.push_back(path);
				}
			}
			else
			{
				settings.filesToSelect.push_back(path);
			}
			continue;
		}
		else if (wArgLower == L"/select")
		{
			if (i + 1 < splitResult.arguments.size())
			{
				std::wstring path = utf8StrToWstr(splitResult.arguments[++i]);
				if (currentDirectory)
				{
					auto absolutePath = TransformUserEnteredPathToAbsolutePathAndNormalize(path,
						currentDirectory.value(), EnvVarsExpansion::DontExpand);
					if (absolutePath)
					{
						settings.filesToSelect.push_back(absolutePath.value());
					}
					else
					{
						settings.filesToSelect.push_back(path);
					}
				}
				else
				{
					settings.filesToSelect.push_back(path);
				}
			}
			continue;
		}

		// If it's not a special / argument, it might be a directory or a standard flag.
		// If it doesn't start with - or /, try to resolve it as a relative path if it's likely a directory.
		if (!arg.empty() && arg[0] != '-' && arg[0] != '/')
		{
			// Check if previous argument was a flag that expects a non-path value
			bool isFlagValue = false;
			if (i > 0)
			{
				const std::string &prevArg = splitResult.arguments[i - 1];
				if (prevArg == "-L" || prevArg == "--lang" || prevArg == "--language" ||
					prevArg == "-f" || prevArg == "--features" || prevArg == "--enable-features" ||
					prevArg == "-n" || prevArg == "--notify" || prevArg == "--change-notify-mode" ||
					prevArg == "-d" || prevArg == "--set-default" || prevArg == "--set-as-default")
				{
					isFlagValue = true;
				}
			}

			if (!isFlagValue && currentDirectory)
			{
				auto absolutePath = TransformUserEnteredPathToAbsolutePathAndNormalize(wArg,
					currentDirectory.value(), EnvVarsExpansion::DontExpand);
				if (absolutePath)
				{
					finalArguments.push_back(wstrToUtf8Str(absolutePath.value()));
					continue;
				}
			}
		}

		finalArguments.push_back(arg);
	}

	// CLI11 requires arguments be provided in reverse order.
	std::reverse(finalArguments.begin(), finalArguments.end());

	try
	{
		app.parse(finalArguments);
	}
	catch (const CLI::ParseError &e)
	{
		auto exitCode = app.exit(e);

		// If we are in a console, we want to return the cursor.
		// Sending a VK_RETURN to the console window can help.
		if (GetConsoleWindow() != nullptr)
		{
			PostMessage(GetConsoleWindow(), WM_KEYDOWN, VK_RETURN, 0);
		}

		return ExitInfo{ exitCode };
	}

	return settings;
}

}
