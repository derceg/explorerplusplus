// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>
#include <vector>

class HistoryModel;
class NavigationEvents;
class NavigationRequest;

class HistoryTracker
{
public:
	HistoryTracker(HistoryModel *historyModel, NavigationEvents *navigationEvents);

private:
	void OnNavigationCommitted(const NavigationRequest *request);

	HistoryModel *const m_historyModel;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
