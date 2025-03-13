// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "HistoryEntry.h"
#include "NavigationController.h"
#include <boost/core/noncopyable.hpp>
#include <boost/signals2.hpp>
#include <vector>

struct NavigateParams;
class NavigationEvents;
class NavigationManager;
class NavigationRequest;
class PreservedHistoryEntry;
class ShellBrowser;
class TabNavigationInterface;

enum class NavigationTargetMode
{
	Normal,
	ForceNewTab
};

class ShellNavigationController :
	public NavigationController<HistoryEntry>,
	private boost::noncopyable
{
public:
	// `initialPidl` here will be used to set up an initial entry. That then means that there will
	// always be a current entry. That is, `GetCurrentEntry` will always return a non-null value.
	ShellNavigationController(const ShellBrowser *shellBrowser,
		NavigationManager *navigationManager, NavigationEvents *navigationEvents,
		TabNavigationInterface *tabNavigation, const PidlAbsolute &initialPidl);

	ShellNavigationController(const ShellBrowser *shellBrowser,
		NavigationManager *navigationManager, NavigationEvents *navigationEvents,
		TabNavigationInterface *tabNavigation,
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries,
		int currentEntry);

	[[nodiscard]] bool CanGoUp() const;
	void GoUp();

	void Refresh();

	void Navigate(const std::wstring &path);
	void Navigate(NavigateParams &navigateParams);

	void SetNavigationTargetMode(NavigationTargetMode navigationTargetMode);
	NavigationTargetMode GetNavigationTargetMode() const;

	HistoryEntry *GetEntryById(int id);

private:
	void Initialize(const ShellBrowser *shellBrowser, NavigationEvents *navigationEvents);

	static std::vector<std::unique_ptr<HistoryEntry>> CopyPreservedHistoryEntries(
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries);

	void Navigate(const HistoryEntry *entry) override;

	void OnNavigationCommitted(const NavigationRequest *request);

	NavigationManager *const m_navigationManager;

	TabNavigationInterface *m_tabNavigation;
	NavigationTargetMode m_navigationTargetMode = NavigationTargetMode::Normal;

	std::vector<boost::signals2::scoped_connection> m_connections;
};
