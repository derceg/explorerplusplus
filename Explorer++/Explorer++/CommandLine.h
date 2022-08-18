// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <variant>

namespace CommandLine
{
	struct Settings
	{
		bool enablePlugins;
		bool registerForShellNotifications;
		std::wstring language;
		std::vector<std::wstring> directories;
	};

	struct ExitInfo
	{
		int exitCode;
	};

	std::variant<Settings, ExitInfo> ProcessCommandLine();
}