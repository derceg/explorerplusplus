// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "HistoryEntry.h"
#include "../Helper/PidlHelper.h"
#include <boost/signals2.hpp>

enum class NavigationType
{
	// Standard navigation to a folder.
	Normal,

	// Navigation to a history item (either backwards or forwards).
	History,

	// Navigation that goes up one level.
	Up
};

struct NavigateParams
{
public:
	// Navigating to a shell link object (i.e. an object that supports the IShellLink interface),
	// will result in a navigation to the target instead. Therefore, the final pidl may not always
	// match the request pidl.
	PidlAbsolute requestPidl;
	PidlAbsolute pidl;

	std::optional<int> historyEntryId;
	bool addHistoryEntry = true;
	NavigationType navigationType = NavigationType::Normal;

	// When navigating up, this will store the pidl of the previous item.
	PidlAbsolute originalPidl;

	static NavigateParams Normal(PCIDLIST_ABSOLUTE pidl, bool addHistoryEntry = true)
	{
		NavigateParams params;
		params.requestPidl = pidl;
		params.pidl = pidl;
		params.addHistoryEntry = addHistoryEntry;
		params.navigationType = NavigationType::Normal;
		return params;
	}

	static NavigateParams History(const HistoryEntry *historyEntry)
	{
		NavigateParams params;
		params.requestPidl = historyEntry->GetPidl().get();
		params.pidl = historyEntry->GetPidl().get();
		params.historyEntryId = historyEntry->GetId();
		params.addHistoryEntry = false;
		params.navigationType = NavigationType::History;
		return params;
	}

	static NavigateParams Up(PCIDLIST_ABSOLUTE pidl, PCIDLIST_ABSOLUTE originalPidl)
	{
		NavigateParams params;
		params.requestPidl = pidl;
		params.pidl = pidl;
		params.originalPidl = originalPidl;
		params.addHistoryEntry = true;
		params.navigationType = NavigationType::Up;
		return params;
	}

private:
	NavigateParams() = default;
};

using NavigationStartedSignal = boost::signals2::signal<void(const NavigateParams &navigateParams)>;
using NavigationCommittedSignal =
	boost::signals2::signal<void(const NavigateParams &navigateParams)>;
using NavigationCompletedSignal =
	boost::signals2::signal<void(const NavigateParams &navigateParams)>;
using NavigationFailedSignal = boost::signals2::signal<void(const NavigateParams &navigateParams)>;

class ShellNavigator
{
public:
	virtual ~ShellNavigator() = default;

	virtual HRESULT Navigate(NavigateParams &navigateParams) = 0;
	virtual boost::signals2::connection AddNavigationStartedObserver(
		const NavigationStartedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) = 0;
	virtual boost::signals2::connection AddNavigationCommittedObserver(
		const NavigationCommittedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) = 0;
	virtual boost::signals2::connection AddNavigationCompletedObserver(
		const NavigationCompletedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) = 0;
	virtual boost::signals2::connection AddNavigationFailedObserver(
		const NavigationFailedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) = 0;
};
