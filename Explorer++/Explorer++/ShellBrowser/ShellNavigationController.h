// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "HistoryEntry.h"
#include "NavigationController.h"
#include "NavigatorInterface.h"
#include "PreservedHistoryEntry.h"
#include "TabNavigationInterface.h"
#include "../Helper/IconFetcher.h"
#include "../Helper/Macros.h"
#include <boost/signals2.hpp>

class ShellNavigationController : public NavigationController<HistoryEntry, HRESULT>
{
public:
	enum class NavigationMode
	{
		Normal,
		ForceNewTab
	};

	ShellNavigationController(NavigatorInterface *navigator, TabNavigationInterface *tabNavigation,
		IconFetcherInterface *iconFetcher);
	ShellNavigationController(NavigatorInterface *navigator, TabNavigationInterface *tabNavigation,
		IconFetcherInterface *iconFetcher,
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries,
		int currentEntry);

	[[nodiscard]] bool CanGoUp() const;
	HRESULT GoUp();

	HRESULT Refresh();

	HRESULT BrowseFolder(const std::wstring &path, bool addHistoryEntry = true);
	HRESULT BrowseFolder(PCIDLIST_ABSOLUTE pidl, bool addHistoryEntry = true);

	void SetNavigationMode(NavigationMode navigationMode);

private:
	DISALLOW_COPY_AND_ASSIGN(ShellNavigationController);

	void Initialize();

	static std::vector<std::unique_ptr<HistoryEntry>> CopyPreservedHistoryEntries(
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries);

	HRESULT BrowseFolder(const HistoryEntry *entry, bool addHistoryEntry = true) override;
	HRESULT GetFailureValue() override;

	void OnNavigationCompleted(PCIDLIST_ABSOLUTE pidlDirectory, bool addHistoryEntry);

	NavigatorInterface *m_navigator;

	TabNavigationInterface *m_tabNavigation;
	NavigationMode m_navigationMode = NavigationMode::Normal;

	IconFetcherInterface *m_iconFetcher;

	std::vector<boost::signals2::scoped_connection> m_connections;
};