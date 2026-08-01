// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ShellHelper.h"
#include <filesystem>

class TabContainer;

class TerminalLauncher
{
public:
	static void OpenCommandPrompt(TabContainer *tabContainer,
		const std::filesystem::path &directory, LaunchProcessFlags flags);
	static bool TryOpenBatchFile(TabContainer *tabContainer, const std::filesystem::path &itemPath,
		const std::wstring &parameters, const std::filesystem::path &workingDirectory);
	static bool TryOpenBatchFile(TabContainer *tabContainer, PCIDLIST_ABSOLUTE pidlItem,
		const std::wstring &parameters, const std::filesystem::path &workingDirectory);
};
