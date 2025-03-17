// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

struct NavigateParams;

// Tabs need to have the ability to open new tabs. They don't, however, need access to the full
// TabContainerImpl interface (e.g. a tab has no need to close a tab or retrieve the selected tab).
// The simple interface here exists purely to allow a tab to create a new tab when necessary.
//
// Note that this function also doesn't allow the caller to customize the new tab in any way.
class TabNavigationInterface
{
public:
	virtual ~TabNavigationInterface() = default;

	virtual void CreateNewTab(NavigateParams &navigateParams, bool selected) = 0;
	virtual void SelectTabById(int tabId) = 0;
};
