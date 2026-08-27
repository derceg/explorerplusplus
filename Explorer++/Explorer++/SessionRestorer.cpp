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
#include "TabStorage.h"
#include "WindowStorage.h"
#include <ranges>

SessionRestorer::SessionRestorer(const CommandLine::Settings *commandLineSettings,
	const Config *config, const FeatureList *featureList,
	BrowserWindowFactory *browserWindowFactory) :
	m_commandLineSettings(commandLineSettings),
	m_config(config),
	m_featureList(featureList),
	m_browserWindowFactory(browserWindowFactory)
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
	auto currentDirectory = GetCurrentDirectoryWrapper();
	CHECK(currentDirectory);

	std::vector<std::wstring> processedPaths;

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

		processedPaths.push_back(*absolutePath);
	}

	if (processedPaths.empty())
	{
		return;
	}

	// When a set of command-line directories is supplied:
	//
	// - The directories will be added to the set of tabs, if the startup mode was set to
	//   StartupMode::PreviousTabs.
	// - The directories will replace the set of tabs otherwise.
	if (m_config->startupMode != +StartupMode::PreviousTabs)
	{
		targetWindow.tabs.clear();
		targetWindow.selectedTab = 0;
	}

	for (const auto &processedPath : processedPaths)
	{
		targetWindow.tabs.push_back({ .directory = processedPath });
	}
}
