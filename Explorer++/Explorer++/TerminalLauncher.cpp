// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TerminalLauncher.h"
#include "CommandPromptProfile.h"
#include "TabContainer.h"
#include <wil/resource.h>

void TerminalLauncher::OpenCommandPrompt(TabContainer *tabContainer,
	const std::filesystem::path &directory, LaunchProcessFlags flags)
{
	if (!WI_IsFlagSet(flags, LaunchProcessFlags::Elevated))
	{
		auto launchRequest = CommandPromptProfile::CreateInteractive(directory);

		if (launchRequest && tabContainer->CreateNewTerminalTab(*launchRequest))
		{
			return;
		}
	}

	// Elevated terminal processes can't be hosted safely inside this unelevated window. Also retain
	// this path as a fallback when Windows Terminal Control or ConPTY isn't available.
	auto commandPromptPath = CommandPromptProfile::GetExecutablePath();

	if (!commandPromptPath)
	{
		return;
	}

	std::wstring parameters;

	if (WI_IsFlagSet(flags, LaunchProcessFlags::Elevated))
	{
		parameters = L"/K cd /d " + directory.wstring();
	}

	LaunchProcess(nullptr, commandPromptPath->c_str(), parameters, directory.wstring(), flags);
}

bool TerminalLauncher::TryOpenBatchFile(TabContainer *tabContainer,
	const std::filesystem::path &itemPath, const std::wstring &parameters,
	const std::filesystem::path &workingDirectory)
{
	auto launchRequest =
		CommandPromptProfile::CreateBatchFile(itemPath, parameters, workingDirectory);
	return launchRequest && tabContainer->CreateNewTerminalTab(*launchRequest);
}

bool TerminalLauncher::TryOpenBatchFile(TabContainer *tabContainer, PCIDLIST_ABSOLUTE pidlItem,
	const std::wstring &parameters, const std::filesystem::path &workingDirectory)
{
	wil::unique_cotaskmem_string itemPath;

	if (FAILED(SHGetNameFromIDList(pidlItem, SIGDN_FILESYSPATH, &itemPath)) || !itemPath)
	{
		return false;
	}

	return TryOpenBatchFile(tabContainer, itemPath.get(), parameters, workingDirectory);
}
