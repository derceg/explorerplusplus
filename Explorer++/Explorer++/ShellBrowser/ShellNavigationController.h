// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "HistoryEntry.h"
#include "NavigationController.h"
#include "../Helper/Macros.h"
#include <boost/signals2.hpp>
#include <vector>

class IconFetcherInterface;
struct NavigateParams;
struct PreservedHistoryEntry;
class ShellNavigator;
class TabNavigationInterface;

enum class NavigationMode
{
	Normal,
	ForceNewTab
};

class ShellNavigationController : public NavigationController<HistoryEntry, HRESULT>
{
public:
	ShellNavigationController(ShellNavigator *navigator, TabNavigationInterface *tabNavigation,
		IconFetcherInterface *iconFetcher);
	ShellNavigationController(ShellNavigator *navigator, TabNavigationInterface *tabNavigation,
		IconFetcherInterface *iconFetcher,
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries,
		int currentEntry);

	HRESULT GoToOffset(int offset) override;

	[[nodiscard]] bool CanGoUp() const;
	HRESULT GoUp();

	HRESULT Refresh();

	HRESULT Navigate(const std::wstring &path);
	HRESULT Navigate(NavigateParams &navigateParams);

	void SetNavigationMode(NavigationMode navigationMode);

	HistoryEntry *GetEntryById(int id);

private:
	DISALLOW_COPY_AND_ASSIGN(ShellNavigationController);

	void Initialize();

	static std::vector<std::unique_ptr<HistoryEntry>> CopyPreservedHistoryEntries(
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries);

	HRESULT Navigate(const HistoryEntry *entry) override;
	HRESULT GetFailureValue() override;

	void OnNavigationCommitted(const NavigateParams &navigateParams);
	std::unique_ptr<HistoryEntry> BuildEntry(const NavigateParams &navigateParams);

	ShellNavigator *m_navigator;

	TabNavigationInterface *m_tabNavigation;
	NavigationMode m_navigationMode = NavigationMode::Normal;

	IconFetcherInterface *m_iconFetcher;

	std::vector<boost::signals2::scoped_connection> m_connections;
};
