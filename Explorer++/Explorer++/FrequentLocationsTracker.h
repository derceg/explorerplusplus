// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>
#include <vector>

class FrequentLocationsModel;
class NavigationEvents;
class NavigationRequest;

class FrequentLocationsTracker
{
public:
	FrequentLocationsTracker(FrequentLocationsModel *model, NavigationEvents *navigationEvents);

private:
	void OnNavigationCommitted(const NavigationRequest *request);

	FrequentLocationsModel *const m_model;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
