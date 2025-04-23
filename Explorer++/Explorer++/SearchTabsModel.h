// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/SignalWrapper.h"
#include <boost/signals2.hpp>
#include <concurrencpp/concurrencpp.h>
#include <string>
#include <vector>

class NavigationEvents;
class ShellBrowserEvents;
class Tab;
class TabEvents;
class TabList;

// Represents the set of all tabs, optionally filtered by a search term.
class SearchTabsModel
{
public:
	// Signals
	SignalWrapper<SearchTabsModel, void()> updatedSignal;

	SearchTabsModel(const TabList *tabList, TabEvents *tabEvents,
		ShellBrowserEvents *shellBrowserEvents, NavigationEvents *navigationEvents);

	// When the search term is empty, no filtering will be applied. Otherwise, the search term will
	// be matched against each tab's name and directory, with only matching tabs being returned by
	// `GetResults`.
	void SetSearchTerm(const std::wstring &searchTerm);
	const std::wstring &GetSearchTerm() const;

	concurrencpp::generator<Tab *> GetResults() const;

private:
	void OnTabsChanged();
	bool TabFilter(const Tab *tab) const;
	static std::wstring GetTabDirectory(const Tab *tab);

	const TabList *const m_tabList;
	std::wstring m_searchTerm;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
