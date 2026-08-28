// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <vector>

class BrowserWindowFactory;
struct Config;
class FeatureList;
class ShellNameParser;
struct WindowStorageData;

namespace CommandLine
{

struct Settings;

}

class SessionRestorer
{
public:
	SessionRestorer(const CommandLine::Settings *commandLineSettings, const Config *config,
		const FeatureList *featureList, BrowserWindowFactory *browserWindowFactory,
		ShellNameParser *shellNameParser);

	void Restore(const std::vector<WindowStorageData> &sessionWindows);

private:
	std::vector<WindowStorageData> GetWindowsToRestore(
		const std::vector<WindowStorageData> &sessionWindows) const;
	void AddStartupModeTabs(WindowStorageData &targetWindow) const;
	void AddCommandLineTabs(WindowStorageData &targetWindow) const;

	const CommandLine::Settings *const m_commandLineSettings;
	const Config *const m_config;
	const FeatureList *const m_featureList;
	BrowserWindowFactory *const m_browserWindowFactory;
	ShellNameParser *const m_shellNameParser;
};
