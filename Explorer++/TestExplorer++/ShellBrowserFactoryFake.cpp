// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellBrowserFactoryFake.h"
#include "ShellBrowserFake.h"

ShellBrowserFactoryFake::ShellBrowserFactoryFake(BrowserWindow *browser,
	NavigationEvents *navigationEvents) :
	m_browser(browser),
	m_navigationEvents(navigationEvents)
{
}

std::unique_ptr<ShellBrowser> ShellBrowserFactoryFake::Create(const PidlAbsolute &initialPidl,
	const FolderSettings &folderSettings, const FolderColumns *initialColumns)
{
	UNREFERENCED_PARAMETER(initialPidl);

	return std::make_unique<ShellBrowserFake>(m_browser, m_navigationEvents, folderSettings,
		initialColumns ? *initialColumns : FolderColumns{});
}

std::unique_ptr<ShellBrowser> ShellBrowserFactoryFake::CreateFromPreserved(
	const std::vector<std::unique_ptr<PreservedHistoryEntry>> &history, int currentEntry,
	const PreservedFolderState &preservedFolderState)
{
	return std::make_unique<ShellBrowserFake>(m_browser, m_navigationEvents, history, currentEntry,
		preservedFolderState);
}
