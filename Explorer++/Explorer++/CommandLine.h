// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Config.h"
#include <variant>

namespace CommandLine
{
	struct Settings
	{
		bool enablePlugins;
		ShellChangeNotificationType shellChangeNotificationType;
		std::wstring language;
		std::vector<std::wstring> directories;
	};

	struct ExitInfo
	{
		int exitCode;
	};

	std::variant<Settings, ExitInfo> ProcessCommandLine();
}
