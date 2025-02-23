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

enum class HistoryEntryType
{
	// No history entry will be added.
	None,

	// A new history entry will be added.
	AddEntry,

	// The current history entry will be replaced. Useful both in situations where the current
	// history entry may be out of date (e.g. a history entry for a past folder may include an out
	// of date display name), as well as situations where the entry needs to be replaced wholesale
	// (e.g. because the current directory has been renamed and the entry needs to be replaced by
	// one that refers to the new location).
	ReplaceCurrentEntry
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
	HistoryEntryType historyEntryType = HistoryEntryType::AddEntry;
	NavigationType navigationType = NavigationType::Normal;

	// if true, the navigation will proceed in the current tab, regardless of the navigation target
	// mode in effect. Useful, for example, in situations where the current directory has been
	// renamed or deleted and a navigation to another directory needs to occur within the tab,
	// regardless of the navigation target mode.
	bool overrideNavigationTargetMode = false;

	// When navigating up, this will store the pidl of the previous item.
	PidlAbsolute originalPidl;

	static NavigateParams Normal(PCIDLIST_ABSOLUTE pidl,
		HistoryEntryType historyEntryType = HistoryEntryType::AddEntry)
	{
		NavigateParams params;
		params.requestPidl = pidl;
		params.pidl = pidl;
		params.historyEntryType = historyEntryType;
		params.navigationType = NavigationType::Normal;
		return params;
	}

	static NavigateParams History(const HistoryEntry *historyEntry)
	{
		NavigateParams params;
		params.requestPidl = historyEntry->GetPidl();
		params.pidl = historyEntry->GetPidl();
		params.historyEntryId = historyEntry->GetId();
		params.historyEntryType = HistoryEntryType::ReplaceCurrentEntry;
		params.navigationType = NavigationType::History;
		return params;
	}

	static NavigateParams Up(PCIDLIST_ABSOLUTE pidl, PCIDLIST_ABSOLUTE originalPidl)
	{
		NavigateParams params;
		params.requestPidl = pidl;
		params.pidl = pidl;
		params.originalPidl = originalPidl;
		params.historyEntryType = HistoryEntryType::AddEntry;
		params.navigationType = NavigationType::Up;
		return params;
	}

	// This is only used in tests.
	bool operator==(const NavigateParams &) const = default;

private:
	NavigateParams() = default;
};

using NavigationStartedSignal = boost::signals2::signal<void(const NavigateParams &navigateParams)>;
using NavigationCommittedSignal =
	boost::signals2::signal<void(const NavigateParams &navigateParams)>;
using NavigationCompletedSignal =
	boost::signals2::signal<void(const NavigateParams &navigateParams)>;
using NavigationFailedSignal = boost::signals2::signal<void(const NavigateParams &navigateParams)>;
using NavigationCancelledSignal =
	boost::signals2::signal<void(const NavigateParams &navigateParams)>;

class ShellNavigator
{
public:
	virtual ~ShellNavigator() = default;

	// Navigations can result in various success or failure states, summarized by the following
	// cases:
	//
	// 1. Initial Navigation (Success)
	//    1.1. The "navigation started" signal is triggered.
	//    1.2. Directory enumeration succeeds.
	//    1.3. The "navigation committed" signal is triggered, making the requested folder the
	//         current folder.
	//    1.4. Items are displayed in the view.
	//    1.5. The "navigation completed" signal is triggered.
	//
	// 2. Initial Navigation (Failure)
	//    2.1. The "navigation started" signal is triggered.
	//    2.2. Directory enumeration fails.
	//    2.3. The "navigation committed" signal is triggered, making the requested folder the
	//         current folder.
	//    2.4. The "navigation completed" signal is triggered.
	//
	// 3. Initial Navigation (Cancellation)
	//    3.1. The "navigation started" signal is triggered.
	//    3.2. The navigation is stopped early.
	//    3.3. The "navigation committed" signal is triggered, making the requested folder the
	//         current folder.
	//    3.4. The "navigation completed" signal is triggered.
	//
	// 4. Subsequent Navigation (Success)
	//    4.1. The "navigation started" signal is triggered.
	//    4.2. Directory enumeration succeeds.
	//    4.3. The "navigation committed" signal is triggered, making the requested folder the
	//         current folder.
	//    4.4. Items are displayed in the view.
	//    4.5. The "navigation completed" signal is triggered.
	//
	// 5. Subsequent Navigation (Failure)
	//    5.1. The "navigation started" signal is triggered.
	//    5.2. Directory enumeration fails.
	//    5.3. The "navigation failed" signal is triggered.
	//
	// 6. Subsequent Navigation (Cancellation)
	//    6.1. The "navigation started" signal is triggered.
	//    6.2. The navigation is stopped early.
	//    6.3. The "navigation cancelled" signal is triggered.

	// Triggered when a navigation is initiated.
	virtual boost::signals2::connection AddNavigationStartedObserver(
		const NavigationStartedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) = 0;

	// Triggered when the enumeration for a directory successfully finishes. At this point, the
	// requested folder has become the current folder, though the enumerated items haven't yet been
	// displayed.
	//
	// This can also be triggered if the initial navigation fails. Typically, if a navigation fails,
	// no folder change will occur. Instead, the original folder will continue to be shown. However,
	// when the first navigation occurs, there is no original folder. Because a folder always needs
	// to be displayed, the only effective option is to consider the failed navigation committed. In
	// that case, this event will be triggered, followed by the completed event. A failed event
	// won't be triggered.
	virtual boost::signals2::connection AddNavigationCommittedObserver(
		const NavigationCommittedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) = 0;

	// Triggered when the enumerated items for a directory have been inserted into the view.
	// Indicates that the navigation has fully completed.
	virtual boost::signals2::connection AddNavigationCompletedObserver(
		const NavigationCompletedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) = 0;

	// Triggered when the enumeration for a navigation fails. As noted above, if the initial
	// navigation fails, this won't be triggered, as the navigation will be committed instead.
	virtual boost::signals2::connection AddNavigationFailedObserver(
		const NavigationFailedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) = 0;

	// Triggered when a navigation is stopped early. As noted above, if the initial navigation is
	// cancelled, this won't be triggered, as the navigation will be committed instead.
	virtual boost::signals2::connection AddNavigationCancelledObserver(
		const NavigationCancelledSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) = 0;
};
