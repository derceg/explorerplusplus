// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SessionRestorer.h"
#include "BrowserWindowFactory.h"
#include "CommandLine.h"
#include "Config.h"
#include "FeatureList.h"
#include "MainRebarStorage.h"
#include "ShellNameParser.h"
#include "TabStorage.h"
#include "WindowStorage.h"
#include "../Helper/ShellHelper.h"
#include <ranges>

SessionRestorer::SessionRestorer(const CommandLine::Settings *commandLineSettings,
	const Config *config, const FeatureList *featureList,
	BrowserWindowFactory *browserWindowFactory, ShellNameParser *shellNameParser) :
	m_commandLineSettings(commandLineSettings),
	m_config(config),
	m_featureList(featureList),
	m_browserWindowFactory(browserWindowFactory),
	m_shellNameParser(shellNameParser)
{
}

void SessionRestorer::Restore(const std::vector<WindowStorageData> &sessionWindows)
{
	auto windowsToRestore = GetWindowsToRestore(sessionWindows);
	CHECK(!windowsToRestore.empty());

	auto &targetWindow = windowsToRestore.back();
	AddStartupModeTabs(targetWindow);

	// This is explicitly done last, since the command-line tabs can overwrite existing tabs.
	AddCommandLineTabs(targetWindow);

	// If this feature isn't enabled, only a single window is supported.
	size_t maxWindowsToRestore =
		m_featureList->IsEnabled(Feature::MultipleWindowsPerSession) ? windowsToRestore.size() : 1;

	for (const auto &windowToRestore : windowsToRestore | std::views::take(maxWindowsToRestore))
	{
		m_browserWindowFactory->CreateBrowserWindow(&windowToRestore);
	}
}

std::vector<WindowStorageData> SessionRestorer::GetWindowsToRestore(
	const std::vector<WindowStorageData> &sessionWindows) const
{
	std::vector<WindowStorageData> windowsToRestore;

	WindowStorageData newWindowData;

	if (!sessionWindows.empty())
	{
		// The details here will be used if a new window needs to be created (as opposed to
		// restoring the previous set of windows).
		newWindowData = sessionWindows[0];
		newWindowData.tabs.clear();
		newWindowData.selectedTab = 0;
	}

	if (m_config->startupMode == +StartupMode::PreviousTabs)
	{
		windowsToRestore = sessionWindows;
	}

	if (windowsToRestore.empty())
	{
		windowsToRestore = { newWindowData };
	}

	return windowsToRestore;
}

void SessionRestorer::AddStartupModeTabs(WindowStorageData &targetWindow) const
{
	switch (m_config->startupMode)
	{
	case StartupMode::PreviousTabs:
		// Nothing needs to be done here. The tabs from the previous session are already available
		// above.
		break;

	case StartupMode::CustomFolders:
		for (const auto &startupFolder : m_config->startupFolders)
		{
			targetWindow.tabs.push_back({ .directory = startupFolder });
		}
		break;

	case StartupMode::DefaultFolder:
		// Nothing needs to be done here. An empty set of tabs will result in a default tab being
		// created.
		break;
	}
}

void SessionRestorer::AddCommandLineTabs(WindowStorageData &targetWindow) const
{
	// It's implicitly assumed that this will succeed. Although the documentation states that
	// GetCurrentDirectory() can fail, I'm not sure under what circumstances it ever would.
	auto currentDirectory = GetCurrentDirectoryWrapper();
	CHECK(currentDirectory);

	std::vector<TabStorageData> commandLineTabs;

	for (const auto &directory : m_commandLineSettings->directories)
	{
		// Windows Explorer doesn't expand environment variables passed in on the command line. The
		// command-line interpreter that's being used can expand variables - for example, running:
		//
		// explorer.exe %windir%
		//
		// from cmd.exe will result in %windir% being expanded before being passed to explorer.exe.
		//
		// But if explorer.exe is launched with the string %windir% passed as a parameter, no
		// expansion will occur.
		//
		// Therefore, no expansion is performed here either.
		//
		// One difference from Explorer is that paths here are trimmed, which means that passing a
		// path like "  C:\Windows  " will result in "C:\Windows" being opened.
		auto absolutePath = TransformUserEnteredPathToAbsolutePathAndNormalize(directory,
			currentDirectory.value(), EnvVarsExpansion::DontExpand);

		if (!absolutePath)
		{
			continue;
		}

		commandLineTabs.push_back({ .directory = *absolutePath });
	}

	for (const auto &fileToSelect : m_commandLineSettings->filesToSelect)
	{
		auto absolutePath = TransformUserEnteredPathToAbsolutePathAndNormalize(fileToSelect,
			currentDirectory.value(), EnvVarsExpansion::DontExpand);

		if (!absolutePath)
		{
			continue;
		}

		auto parseResult = m_shellNameParser->ParseDisplayName(*absolutePath);

		if (!parseResult)
		{
			continue;
		}

		auto parentPidl = parseResult.value();
		parentPidl.RemoveLastItem();

		commandLineTabs.push_back({ .pidl = parentPidl, .itemsToSelect = { parseResult.value() } });
	}

	if (commandLineTabs.empty())
	{
		return;
	}

	// When one or more tabs are specified on the command line:
	//
	// - The tabs are added to the restored tabs when using StartupMode::PreviousTabs.
	// - The tabs replace the startup tabs otherwise.
	if (m_config->startupMode != +StartupMode::PreviousTabs)
	{
		targetWindow.tabs.clear();
		targetWindow.selectedTab = 0;
	}

	targetWindow.tabs.insert(targetWindow.tabs.end(),
		std::make_move_iterator(commandLineTabs.begin()),
		std::make_move_iterator(commandLineTabs.end()));
}
