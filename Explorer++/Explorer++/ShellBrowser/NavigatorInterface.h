// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>

typedef boost::signals2::signal<void(PCIDLIST_ABSOLUTE pidlDirectory, bool addHistoryEntry)> NavigationCompletedSignal;

__interface NavigatorInterface
{
	HRESULT BrowseFolder(PCIDLIST_ABSOLUTE pidlDirectory, bool addHistoryEntry = true);
	boost::signals2::connection AddNavigationCompletedObserver(const NavigationCompletedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back);
};