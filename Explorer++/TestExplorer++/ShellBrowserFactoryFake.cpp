// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellBrowserFactoryFake.h"
#include "ShellBrowserFake.h"
#include "TabNavigationMock.h"

ShellBrowserFactoryFake::ShellBrowserFactoryFake(NavigationEvents *navigationEvents,
	TabNavigationMock *tabNavigation) :
	m_navigationEvents(navigationEvents),
	m_tabNavigation(tabNavigation)
{
}

std::unique_ptr<ShellBrowser> ShellBrowserFactoryFake::Create(const PidlAbsolute &initialPidl,
	const FolderSettings &folderSettings, const FolderColumns *initialColumns)
{
	UNREFERENCED_PARAMETER(initialPidl);
	UNREFERENCED_PARAMETER(folderSettings);
	UNREFERENCED_PARAMETER(initialColumns);

	return std::make_unique<ShellBrowserFake>(m_navigationEvents, m_tabNavigation);
}

std::unique_ptr<ShellBrowser> ShellBrowserFactoryFake::CreateFromPreserved(
	const std::vector<std::unique_ptr<PreservedHistoryEntry>> &history, int currentEntry,
	const PreservedFolderState &preservedFolderState)
{
	UNREFERENCED_PARAMETER(preservedFolderState);

	return std::make_unique<ShellBrowserFake>(m_navigationEvents, m_tabNavigation, history,
		currentEntry);
}
