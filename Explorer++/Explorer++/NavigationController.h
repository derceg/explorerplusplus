// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "HistoryEntry.h"
#include "PreservedHistoryEntry.h"
#include "ShellBrowser/ShellBrowser.h"
#include "TabNavigationInterface.h"
#include <boost/signals2.hpp>

class NavigationController
{
public:

	enum class NavigationMode
	{
		Normal,
		ForceNewTab
	};

	NavigationController(CShellBrowser *shellBrowser, TabNavigationInterface *tabNavigation);
	NavigationController(CShellBrowser *shellBrowser, TabNavigationInterface *tabNavigation,
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries, int currentEntry);

	int GetNumHistoryEntries() const;
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

	HRESULT BrowseFolder(const std::wstring &path);
	HRESULT BrowseFolder(PCIDLIST_ABSOLUTE pidl);

	void SetNavigationMode(NavigationMode navigationMode);

private:

	void Initialize();

	static std::vector<std::unique_ptr<HistoryEntry>> NavigationController::CopyPreservedHistoryEntries(
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries);

	void OnNavigationCompleted(PCIDLIST_ABSOLUTE pidlDirectory, bool addHistoryEntry);
	void AddEntry(std::unique_ptr<HistoryEntry> entry);
	HistoryEntry *GetEntryAndUpdateIndex(int offset);

	HRESULT BrowseFolder(PCIDLIST_ABSOLUTE pidl, bool addHistoryEntry);

	std::vector<std::unique_ptr<HistoryEntry>> m_entries;
	int m_currentEntry;

	CShellBrowser *m_shellBrowser;

	TabNavigationInterface *m_tabNavigation;
	NavigationMode m_navigationMode = NavigationMode::Normal;

	std::vector<boost::signals2::scoped_connection> m_connections;
};