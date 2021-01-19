// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

// Tabs need to have the ability to open new tabs. They don't, however, need
// access to the full TabContainer interface (e.g. a tab has no need to close a
// tab or retrieve the selected tab). The simple interface here exists purely to
// allow a tab to create a new tab when necessary.
// Note that this function also doesn't allow the caller to customize the new
// tab in any way.
__interface TabNavigationInterface
{
	void CreateNewTab(PCIDLIST_ABSOLUTE pidlDirectory, bool selected);
};