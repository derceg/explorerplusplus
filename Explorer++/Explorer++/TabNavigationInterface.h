// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

struct NavigateParams;

// Tabs need to have the ability to create new tabs and select tabs. They don't, however, need
// access to the full TabContainer interface (e.g. a tab has no need to close a tab or retrieve the
// selected tab), which is why this interface exists.
class TabNavigationInterface
{
public:
	virtual ~TabNavigationInterface() = default;

	virtual void CreateNewTab(NavigateParams &navigateParams, bool selected) = 0;
	virtual void SelectTabById(int tabId) = 0;
};
