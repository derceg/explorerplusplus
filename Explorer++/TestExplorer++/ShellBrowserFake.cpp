// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellBrowserFake.h"
#include "ShellBrowser/FolderSettings.h"
#include "ShellBrowser/ShellBrowserHelper.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellTestHelper.h"

ShellBrowserFake::ShellBrowserFake(TabNavigationInterface *tabNavigation,
	const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries, int currentEntry)
{
	m_navigationController = std::make_unique<ShellNavigationController>(this, tabNavigation,
		preservedEntries, currentEntry);
}

ShellBrowserFake::ShellBrowserFake(TabNavigationInterface *tabNavigation)
{
	m_navigationController = std::make_unique<ShellNavigationController>(this, tabNavigation);
}

ShellBrowserFake::~ShellBrowserFake() = default;

// Although the ShellNavigationController can navigate to a path (by transforming it into a pidl),
// it requires that the path exist. This function will transform the path into a simple pidl, which
// doesn't require the path to exist.
HRESULT ShellBrowserFake::NavigateToPath(const std::wstring &path, HistoryEntryType addHistoryType,
	PidlAbsolute *outputPidl)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(path);
	auto navigateParams = NavigateParams::Normal(pidl.Raw(), addHistoryType);
	HRESULT hr = m_navigationController->Navigate(navigateParams);

	if (outputPidl)
	{
		*outputPidl = pidl;
	}

	return hr;
}

FolderSettings ShellBrowserFake::GetFolderSettings() const
{
	return {};
}

ShellNavigationController *ShellBrowserFake::GetNavigationController() const
{
	return m_navigationController.get();
}

void ShellBrowserFake::AddHelper(std::unique_ptr<ShellBrowserHelperBase> helper)
{
	m_helpers.push_back(std::move(helper));
}

HRESULT ShellBrowserFake::Navigate(NavigateParams &navigateParams)
{
	m_navigationStartedSignal(navigateParams);
	m_navigationCommittedSignal(navigateParams);
	m_navigationCompletedSignal(navigateParams);
	return S_OK;
}

boost::signals2::connection ShellBrowserFake::AddNavigationStartedObserver(
	const NavigationStartedSignal::slot_type &observer, boost::signals2::connect_position position)
{
	return m_navigationStartedSignal.connect(observer, position);
}

boost::signals2::connection ShellBrowserFake::AddNavigationCommittedObserver(
	const NavigationCommittedSignal::slot_type &observer,
	boost::signals2::connect_position position)
{
	return m_navigationCommittedSignal.connect(observer, position);
}

boost::signals2::connection ShellBrowserFake::AddNavigationCompletedObserver(
	const NavigationCompletedSignal::slot_type &observer,
	boost::signals2::connect_position position)
{
	return m_navigationCompletedSignal.connect(observer, position);
}

boost::signals2::connection ShellBrowserFake::AddNavigationFailedObserver(
	const NavigationFailedSignal::slot_type &observer, boost::signals2::connect_position position)
{
	return m_navigationFailedSignal.connect(observer, position);
}
