// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/ShellBrowserFactory.h"

class NavigationEvents;
class TabNavigationMock;

class ShellBrowserFactoryFake : public ShellBrowserFactory
{
public:
	ShellBrowserFactoryFake(NavigationEvents *navigationEvents, TabNavigationMock *tabNavigation);

	std::unique_ptr<ShellBrowser> Create(const PidlAbsolute &initialPidl,
		const FolderSettings &folderSettings, const FolderColumns *initialColumns) override;
	std::unique_ptr<ShellBrowser> CreateFromPreserved(
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &history, int currentEntry,
		const PreservedFolderState &preservedFolderState) override;

private:
	NavigationEvents *const m_navigationEvents;
	TabNavigationMock *const m_tabNavigation;
};
