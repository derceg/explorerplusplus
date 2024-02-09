// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/ShellBrowserInterface.h"
#include "../Helper/ShellHelper.h"

class IconFetcher;
struct PreservedHistoryEntry;
class ShellNavigationController;
class TabNavigationInterface;

class ShellBrowserFake : public ShellBrowserInterface
{
public:
	ShellBrowserFake(TabNavigationInterface *tabNavigation, IconFetcher *iconFetcher,
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries,
		int currentEntry);
	ShellBrowserFake(TabNavigationInterface *tabNavigation, IconFetcher *iconFetcher);
	~ShellBrowserFake();

	HRESULT NavigateToPath(const std::wstring &path,
		HistoryEntryType addHistoryType = HistoryEntryType::AddEntry,
		unique_pidl_absolute *outputPidl = nullptr);

	// ShellBrowserInterface
	ShellNavigationController *GetNavigationController() const override;
	void AddHelper(std::unique_ptr<ShellBrowserHelperBase> helper) override;

	// ShellNavigator
	HRESULT Navigate(NavigateParams &navigateParams) override;
	boost::signals2::connection AddNavigationStartedObserver(
		const NavigationStartedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;
	boost::signals2::connection AddNavigationCommittedObserver(
		const NavigationCommittedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;
	boost::signals2::connection AddNavigationCompletedObserver(
		const NavigationCompletedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;
	boost::signals2::connection AddNavigationFailedObserver(
		const NavigationFailedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;

private:
	std::unique_ptr<ShellNavigationController> m_navigationController;
	std::vector<std::unique_ptr<ShellBrowserHelperBase>> m_helpers;

	NavigationStartedSignal m_navigationStartedSignal;
	NavigationCommittedSignal m_navigationCommittedSignal;
	NavigationCompletedSignal m_navigationCompletedSignal;
	NavigationFailedSignal m_navigationFailedSignal;
};
