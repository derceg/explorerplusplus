// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "HistoryEntry.h"
#include "NavigatorInterface.h"
#include "PreservedHistoryEntry.h"
#include "TabNavigationInterface.h"
#include "../Helper/IconFetcher.h"
#include "../Helper/Macros.h"
#include <boost/signals2.hpp>

class NavigationController
{
public:

	enum class NavigationMode
	{
		Normal,
		ForceNewTab
	};

	NavigationController(NavigatorInterface *navigator, TabNavigationInterface *tabNavigation,
		IconFetcher *iconFetcher);
	NavigationController(NavigatorInterface *navigator, TabNavigationInterface *tabNavigation,
		IconFetcher *iconFetcher, const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries,
		int currentEntry);

	int GetNumHistoryEntries() const;
	HistoryEntry *GetCurrentEntry() const;
	int GetCurrentIndex() const;
	HistoryEntry *GetEntry(int offset) const;
	HistoryEntry *GetEntryAtIndex(int index) const;

	bool CanGoBack() const;
	bool CanGoForward() const;
	bool CanGoUp() const;
	std::vector<HistoryEntry *> GetBackHistory() const;
	std::vector<HistoryEntry *> GetForwardHistory() const;

	HRESULT GoBack();
	HRESULT GoForward();
	HRESULT GoToOffset(int offset);
	HRESULT GoUp();

	HRESULT Refresh();

	HRESULT BrowseFolder(const std::wstring &path, bool addHistoryEntry = true);
	HRESULT BrowseFolder(PCIDLIST_ABSOLUTE pidl, bool addHistoryEntry = true);

	void SetNavigationMode(NavigationMode navigationMode);

private:

	DISALLOW_COPY_AND_ASSIGN(NavigationController);

	void Initialize();

	static std::vector<std::unique_ptr<HistoryEntry>> NavigationController::CopyPreservedHistoryEntries(
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries);

	void OnNavigationCompleted(PCIDLIST_ABSOLUTE pidlDirectory, bool addHistoryEntry);
	void AddEntry(std::unique_ptr<HistoryEntry> entry);
	HistoryEntry *GetEntryAndUpdateIndex(int offset);

	std::vector<std::unique_ptr<HistoryEntry>> m_entries;
	int m_currentEntry;


	NavigatorInterface *m_navigator;

	TabNavigationInterface *m_tabNavigation;
	NavigationMode m_navigationMode = NavigationMode::Normal;

	IconFetcher *m_iconFetcher;

	std::vector<boost::signals2::scoped_connection> m_connections;
};