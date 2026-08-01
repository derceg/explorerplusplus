// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <filesystem>
#include <string>

enum class TerminalExitBehavior
{
	KeepTabOpen,
	CloseTab
};

struct TerminalProcessLaunchInfo
{
	std::filesystem::path application;
	std::wstring commandLine;
	std::filesystem::path workingDirectory;
};

struct TerminalLaunchRequest
{
	std::filesystem::path initialDirectory;
	TerminalProcessLaunchInfo process;
	TerminalExitBehavior exitBehavior = TerminalExitBehavior::KeepTabOpen;
};
