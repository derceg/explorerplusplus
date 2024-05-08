// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Config.h"
#include "Feature.h"
#include <optional>
#include <variant>

namespace CommandLine
{

struct Settings
{
	bool enableLogging = false;
	std::set<Feature> enableFeatures;
	std::optional<ShellChangeNotificationType> shellChangeNotificationType;
	std::wstring language;
	bool createJumplistTab = false;
	std::vector<std::wstring> filesToSelect;
	std::vector<std::wstring> directories;
};

struct ExitInfo
{
	int exitCode;
};

// Internal command line arguments.
const TCHAR JUMPLIST_TASK_NEWTAB_ARGUMENT[] = _T("--open-new-tab");
const TCHAR APPLICATION_CRASHED_ARGUMENT[] = _T("--application-crashed");

std::variant<Settings, ExitInfo> ProcessCommandLine();

}
