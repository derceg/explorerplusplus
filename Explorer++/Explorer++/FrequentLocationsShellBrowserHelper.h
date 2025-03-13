// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/ShellBrowserHelper.h"
#include <boost/signals2.hpp>
#include <vector>

class FrequentLocationsModel;
class NavigationEvents;

class FrequentLocationsShellBrowserHelper :
	public ShellBrowserHelper<FrequentLocationsShellBrowserHelper>
{
public:
	FrequentLocationsShellBrowserHelper(ShellBrowser *shellBrowser, FrequentLocationsModel *model,
		NavigationEvents *navigationEvents);

private:
	void OnNavigationCommitted(const NavigationRequest *request);

	FrequentLocationsModel *const m_model;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
