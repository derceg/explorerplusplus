// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/signals2.hpp>
#include <concurrencpp/concurrencpp.h>
#include <vector>

class BrowserWindow;
class Tab;
class TabEvents;

// This represents a point at which a tab was activated. The only operation that's supported is
// comparison. That is, this says nothing about when (in wall clock time) a tab was activated,
// simply that if the activation point for tab A is greater than the activation point for tab B,
// then tab A was activated after tab B.
class TabActivationPoint
{
public:
	auto operator<=>(const TabActivationPoint &) const = default;

private:
	friend class TabList;

	TabActivationPoint() = default;

	explicit TabActivationPoint(int value) : m_value(value)
	{
	}

	int m_value = 0;
};

// Maintains a global list of tabs. This class allows clients to retrieve a list of all tabs, or a
// list of tabs in a specific browser window, using a single, unified, interface. Without this,
// retrieving a list of all tabs would be a completely different operation to retrieving a list of
// tabs in an individual browser window.
class TabList
{
public:
	TabList(TabEvents *tabEvents);

	Tab *GetById(int id) const;
	Tab *MaybeGetById(int id) const;
	TabActivationPoint GetTabLastActivationPoint(const Tab *tab) const;
	concurrencpp::generator<Tab *> GetAll() const;
	concurrencpp::generator<Tab *> GetAllByLastActivation() const;
	concurrencpp::generator<Tab *> GetForBrowser(const BrowserWindow *browser) const;

private:
	class TabData
	{
	public:
		TabData(Tab *tab);

		const Tab *GetTab() const;
		Tab *GetMutableTab() const;
		TabActivationPoint GetLastActivationPoint() const;
		void SetLastActivationPoint(TabActivationPoint activationPoint);

	private:
		Tab *const m_tab;
		TabActivationPoint m_lastActivationPoint;
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

	struct ByActivationPoint
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
			// An index of tabs, sorted in descending order of last activation (i.e. most recently
			// activated first).
			boost::multi_index::ordered_non_unique<
				boost::multi_index::tag<ByActivationPoint>,
				boost::multi_index::const_mem_fun<TabData, TabActivationPoint,
					&TabData::GetLastActivationPoint>,
				std::greater<TabActivationPoint>
			>
		>
	>;
	// clang-format on

	void OnTabCreated(Tab &tab);
	void OnTabSelected(const Tab &tab);
	void OnTabRemoved(const Tab &tab);
	static Tab *ExtractTab(const TabData &tabData);

	TabListContainer m_tabs;
	int m_nextActivationPoint = 1;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
