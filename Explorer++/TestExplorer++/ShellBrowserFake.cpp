// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellBrowserFake.h"
#include "ShellBrowser/FolderSettings.h"
#include "ShellBrowser/ShellBrowserHelper.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellEnumeratorFake.h"
#include "ShellTestHelper.h"

ShellBrowserFake::ShellBrowserFake(TabNavigationInterface *tabNavigation,
	const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries, int currentEntry,
	std::shared_ptr<concurrencpp::executor> enumerationExecutor,
	std::shared_ptr<concurrencpp::executor> originalExecutor) :
	ShellBrowserFake(tabNavigation, enumerationExecutor, originalExecutor)
{
	m_navigationController = std::make_unique<ShellNavigationController>(&m_navigationManager,
		tabNavigation, preservedEntries, currentEntry);
}

ShellBrowserFake::ShellBrowserFake(TabNavigationInterface *tabNavigation,
	std::shared_ptr<concurrencpp::executor> enumerationExecutor,
	std::shared_ptr<concurrencpp::executor> originalExecutor) :
	m_shellEnumerator(std::make_unique<ShellEnumeratorFake>()),
	m_inlineExecutor(std::make_shared<concurrencpp::inline_executor>()),
	m_navigationManager(m_shellEnumerator,
		enumerationExecutor ? enumerationExecutor : m_inlineExecutor,
		originalExecutor ? originalExecutor : m_inlineExecutor),
	m_navigationController(
		std::make_unique<ShellNavigationController>(&m_navigationManager, tabNavigation))
{
	m_navigationManager.AddNavigationStartedObserver([this](const NavigateParams &navigateParams)
		{ m_navigationStartedSignal(navigateParams); });
	m_navigationManager.AddNavigationCommittedObserver([this](const NavigateParams &navigateParams)
		{ m_navigationCommittedSignal(navigateParams); });
	m_navigationManager.AddNavigationCompletedObserver([this](const NavigateParams &navigateParams)
		{ m_navigationCompletedSignal(navigateParams); });
	m_navigationManager.AddNavigationFailedObserver(
		[this](const NavigateParams &navigateParams) { m_navigationFailedSignal(navigateParams); });
}

ShellBrowserFake::~ShellBrowserFake()
{
	m_inlineExecutor->shutdown();
}

// Although the ShellNavigationController can navigate to a path (by transforming it into a pidl),
// it requires that the path exist. This function will transform the path into a simple pidl, which
// doesn't require the path to exist.
void ShellBrowserFake::NavigateToPath(const std::wstring &path, HistoryEntryType addHistoryType,
	PidlAbsolute *outputPidl)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(path);
	auto navigateParams = NavigateParams::Normal(pidl.Raw(), addHistoryType);
	m_navigationController->Navigate(navigateParams);

	if (outputPidl)
	{
		*outputPidl = pidl;
	}
}

NavigationManager *ShellBrowserFake::GetNavigationManager()
{
	return &m_navigationManager;
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
