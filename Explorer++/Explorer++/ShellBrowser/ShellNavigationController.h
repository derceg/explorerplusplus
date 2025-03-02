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
class NavigationManager;
class NavigationRequest;
class PreservedHistoryEntry;
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
	ShellNavigationController(NavigationManager *navigationManager,
		TabNavigationInterface *tabNavigation, const PidlAbsolute &initialPidl);

	ShellNavigationController(NavigationManager *navigationManager,
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
	void Initialize();

	static std::vector<std::unique_ptr<HistoryEntry>> CopyPreservedHistoryEntries(
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries);

	void Navigate(const HistoryEntry *entry) override;

	void OnNavigationCommitted(const NavigationRequest *request,
		const std::vector<PidlChild> &items);

	NavigationManager *const m_navigationManager;

	TabNavigationInterface *m_tabNavigation;
	NavigationTargetMode m_navigationTargetMode = NavigationTargetMode::Normal;

	std::vector<boost::signals2::scoped_connection> m_connections;
};
