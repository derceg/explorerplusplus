// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "TerminalLaunchRequest.h"
#include <optional>

class CommandPromptProfile
{
public:
	static std::optional<TerminalLaunchRequest> CreateInteractive(
		const std::filesystem::path &directory);
	static std::optional<TerminalLaunchRequest> CreateBatchFile(
		const std::filesystem::path &itemPath, const std::wstring &parameters,
		const std::filesystem::path &workingDirectory);
};
