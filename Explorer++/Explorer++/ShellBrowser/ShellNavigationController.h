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
class PreservedHistoryEntry;
class ShellNavigator;
class TabNavigationInterface;

enum class NavigationMode
{
	Normal,
	ForceNewTab
};

class ShellNavigationController :
	public NavigationController<HistoryEntry>,
	private boost::noncopyable
{
public:
	ShellNavigationController(ShellNavigator *navigator, TabNavigationInterface *tabNavigation);
	ShellNavigationController(ShellNavigator *navigator, TabNavigationInterface *tabNavigation,
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries,
		int currentEntry);

	[[nodiscard]] bool CanGoUp() const;
	void GoUp();

	void Refresh();

	void Navigate(const std::wstring &path);
	void Navigate(NavigateParams &navigateParams);

	void SetNavigationMode(NavigationMode navigationMode);
	NavigationMode GetNavigationMode() const;

	HistoryEntry *GetEntryById(int id);

private:
	void Initialize();

	static std::vector<std::unique_ptr<HistoryEntry>> CopyPreservedHistoryEntries(
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries);

	void Navigate(const HistoryEntry *entry) override;

	void OnNavigationStarted(const NavigateParams &navigateParams);
	void OnNavigationCommitted(const NavigateParams &navigateParams);

	ShellNavigator *m_navigator;

	TabNavigationInterface *m_tabNavigation;
	NavigationMode m_navigationMode = NavigationMode::Normal;

	std::vector<boost::signals2::scoped_connection> m_connections;
};
