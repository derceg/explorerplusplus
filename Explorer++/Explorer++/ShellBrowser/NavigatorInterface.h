// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>

class HistoryEntry;

using NavigationStartedSignal = boost::signals2::signal<void(PCIDLIST_ABSOLUTE pidl)>;
using NavigationCommittedSignal =
	boost::signals2::signal<void(PCIDLIST_ABSOLUTE pidl, bool addHistoryEntry)>;
using NavigationCompletedSignal = boost::signals2::signal<void(PCIDLIST_ABSOLUTE pidlDirectory)>;
using NavigationFailedSignal = boost::signals2::signal<void()>;

__interface NavigatorInterface
{
	HRESULT BrowseFolder(const HistoryEntry &entry);
	HRESULT BrowseFolder(PCIDLIST_ABSOLUTE pidlDirectory, bool addHistoryEntry = true);
	boost::signals2::connection AddNavigationStartedObserver(
		const NavigationStartedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back);
	boost::signals2::connection AddNavigationCommittedObserver(
		const NavigationCommittedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back);
	boost::signals2::connection AddNavigationCompletedObserver(
		const NavigationCompletedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back);
	boost::signals2::connection AddNavigationFailedObserver(
		const NavigationFailedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back);
};