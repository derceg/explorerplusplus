// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/signals2.hpp>
#include <concurrencpp/concurrencpp.h>
#include <chrono>
#include <vector>

class BrowserWindow;
class Tab;
class TabEvents;

// Maintains a global list of tabs. This class allows clients to retrieve a list of all tabs, or a
// list of tabs in a specific browser window, using a single, unified, interface. Without this,
// retrieving a list of all tabs would be a completely different operation to retrieving a list of
// tabs in an individual browser window.
class TabList
{
public:
	TabList(TabEvents *tabEvents);

	Tab *GetById(int id) const;
	concurrencpp::generator<Tab *> GetAll() const;
	concurrencpp::generator<Tab *> GetAllByLastActiveTime() const;
	concurrencpp::generator<Tab *> GetForBrowser(const BrowserWindow *browser) const;

private:
	class TabData
	{
	public:
		using Clock = std::chrono::steady_clock;

		TabData(Tab *tab);

		const Tab *GetTab() const;
		Tab *GetMutableTab() const;
		Clock::time_point GetLastActiveTime() const;
		void UpdateLastActiveTime();

	private:
		Tab *const m_tab;
		Clock::time_point m_lastActiveTime;
	};

	struct ByTab
	{
	};

	struct ById
	{
	};

	struct ByBrowser
	{
	};

	struct ByActiveTime
	{
	};

	struct TabIdExtractor
	{
		using result_type = int;
		result_type operator()(const TabData &tabData) const;
	};

	struct TabBrowserExtractor
	{
		using result_type = const BrowserWindow *;
		result_type operator()(const TabData &tabData) const;
	};

	// clang-format off
	using TabListContainer = boost::multi_index_container<TabData,
		boost::multi_index::indexed_by<
			// A non-sorted index of unique tabs.
			boost::multi_index::hashed_unique<
				boost::multi_index::tag<ByTab>,
				boost::multi_index::const_mem_fun<TabData, const Tab *, &TabData::GetTab>
			>,
			// A non-sorted index of tabs, based on their unique ID.
			boost::multi_index::hashed_unique<
				boost::multi_index::tag<ById>,
				TabIdExtractor
			>,
			// A non-sorted index of tabs, based on their browser.
			boost::multi_index::hashed_non_unique<
				boost::multi_index::tag<ByBrowser>,
				TabBrowserExtractor
			>,
			// An index of tabs, sorted in descending order of the last active time (i.e. most
			// recently activated first).
			boost::multi_index::ordered_non_unique<
				boost::multi_index::tag<ByActiveTime>,
				boost::multi_index::const_mem_fun<TabData, TabData::Clock::time_point,
					&TabData::GetLastActiveTime>,
				std::greater<TabData::Clock::time_point>
			>
		>
	>;
	// clang-format on

	void OnTabCreated(Tab &tab);
	void OnTabSelected(const Tab &tab);
	void OnTabRemoved(const Tab &tab);
	static Tab *ExtractTab(const TabData &tabData);

	TabListContainer m_tabs;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
