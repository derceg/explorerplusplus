// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "SignalWrapper.h"
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index_container.hpp>
#include <concurrencpp/concurrencpp.h>
#include <chrono>

class BrowserWindow;

// Maintains an unordered list of top-level browser windows. The last active browser window can also
// be retrieved.
class BrowserList
{
public:
	// Note that the browser window should be in a fully constructed state when passed to both of
	// these functions. That is, AddBrowser() shouldn't be called until the window is completely
	// constructed and RemoveBrowser() should be called before the window destruction begins.
	void AddBrowser(BrowserWindow *browser);
	void RemoveBrowser(BrowserWindow *browser);

	concurrencpp::generator<BrowserWindow *> GetList() const;
	BrowserWindow *GetLastActive() const;
	void SetLastActive(BrowserWindow *browser);
	size_t GetSize() const;
	bool IsEmpty() const;

	// Signals
	SignalWrapper<BrowserList, void(BrowserWindow *browser)> browserAddedSignal;
	SignalWrapper<BrowserList, void(BrowserWindow *browser)> willRemoveBrowserSignal;
	SignalWrapper<BrowserList, void(BrowserWindow *browser)> browserRemovedSignal;

private:
	class BrowserData
	{
	public:
		using Clock = std::chrono::system_clock;

		BrowserData(BrowserWindow *browser);

		BrowserWindow *GetBrowser() const;
		Clock::time_point GetLastActiveTime() const;
		void UpdateLastActiveTime();

	private:
		BrowserWindow *const m_browser;
		Clock::time_point m_lastActiveTime;
	};

	struct ByBrowser
	{
	};

	struct ByActiveTime
	{
	};

	// clang-format off
	using BrowserListContainer = boost::multi_index_container<BrowserData,
		boost::multi_index::indexed_by<
			// A non-sorted index of unique browsers.
			boost::multi_index::hashed_unique<
				boost::multi_index::tag<ByBrowser>,
				boost::multi_index::const_mem_fun<BrowserData, BrowserWindow *, &BrowserData::GetBrowser>
			>,
			// An index of browsers, sorted in descending order of the last active time (i.e. most
			// recently activated first).
			boost::multi_index::ordered_non_unique<
				boost::multi_index::tag<ByActiveTime>,
				boost::multi_index::const_mem_fun<BrowserData, BrowserData::Clock::time_point,
					&BrowserData::GetLastActiveTime>,
				std::greater<BrowserData::Clock::time_point>
			>
		>
	>;
	// clang-format on

	BrowserListContainer m_browsers;
};
