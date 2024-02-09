// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellBrowserFake.h"
#include "ShellBrowser/ShellBrowserHelper.h"
#include "ShellBrowser/ShellNavigationController.h"

ShellBrowserFake::ShellBrowserFake(TabNavigationInterface *tabNavigation, IconFetcher *iconFetcher,
	const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries, int currentEntry)
{
	m_navigationController = std::make_unique<ShellNavigationController>(this, tabNavigation,
		iconFetcher, preservedEntries, currentEntry);
}

ShellBrowserFake::ShellBrowserFake(TabNavigationInterface *tabNavigation, IconFetcher *iconFetcher)
{
	m_navigationController =
		std::make_unique<ShellNavigationController>(this, tabNavigation, iconFetcher);
}

ShellBrowserFake::~ShellBrowserFake() = default;

// Although the ShellNavigationController can navigate to a path (by transforming it into a pidl),
// it requires that the path exist. This function will transform the path into a simple pidl, which
// doesn't require the path to exist.
HRESULT ShellBrowserFake::NavigateToPath(const std::wstring &path, HistoryEntryType addHistoryType,
	unique_pidl_absolute *outputPidl)
{
	unique_pidl_absolute pidl(SHSimpleIDListFromPath(path.c_str()));

	if (!pidl)
	{
		return E_FAIL;
	}

	auto navigateParams = NavigateParams::Normal(pidl.get(), addHistoryType);
	HRESULT hr = m_navigationController->Navigate(navigateParams);

	if (outputPidl)
	{
		*outputPidl = std::move(pidl);
	}

	return hr;
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
