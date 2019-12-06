// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/HistoryEntry.h"
#include "ShellBrowser/ShellBrowser.h"
#include <boost/signals2.hpp>

class NavigationController
{
public:

	NavigationController(CShellBrowser *shellBrowser);

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

private:

	void OnNavigationCompleted(PCIDLIST_ABSOLUTE pidlDirectory, bool addHistoryEntry);
	void AddEntry(std::unique_ptr<HistoryEntry> entry);
	HistoryEntry *GetEntryAndUpdateIndex(int offset);

	std::vector<std::unique_ptr<HistoryEntry>> m_entries;
	int m_currentEntry;

	CShellBrowser *m_shellBrowser;

	std::vector<boost::signals2::scoped_connection> m_connections;
};