// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CrashHandlerHelper.h"
#include "Feature.h"
#include "ShellChangeNotificationType.h"
#include "../Helper/SetDefaultFileManager.h"
#include <optional>
#include <variant>

namespace CommandLine
{

struct Settings
{
	bool enableLogging = false;
	std::set<Feature> featuresToEnable;
	std::optional<ShellChangeNotificationType> shellChangeNotificationType;
	std::wstring language;
	bool clearRegistrySettings = false;
	bool removeAsDefault = false;
	DefaultFileManager::ReplaceExplorerMode replaceExplorerMode =
		DefaultFileManager::ReplaceExplorerMode::None;
	bool jumplistNewTab = false;
	std::optional<CrashedData> crashedData;
	std::optional<std::wstring> pasteSymLinksDestination;
	std::vector<std::wstring> filesToSelect;
	std::vector<std::wstring> directories;
};

struct ExitInfo
{
	int exitCode;
};

// Internal command line arguments.
const wchar_t JUMPLIST_TASK_NEWTAB_ARGUMENT[] = L"--open-new-tab";
const wchar_t APPLICATION_CRASHED_ARGUMENT[] = L"--application-crashed";
const wchar_t PASTE_SYMLINKS_ARGUMENT[] = L"--paste-symlinks";

std::variant<Settings, ExitInfo> Parse(const std::wstring &commandLine);

}
