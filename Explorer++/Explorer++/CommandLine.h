// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Config.h"
#include <optional>
#include <variant>

namespace CommandLine
{
struct Settings
{
	bool enablePlugins = false;
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

std::variant<Settings, ExitInfo> ProcessCommandLine();
}
