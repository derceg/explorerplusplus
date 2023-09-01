// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class TabContainer;

// Each browser pane contains a set of tabs, with each tab showing a file listing.
class BrowserPane
{
public:
	BrowserPane(TabContainer *tabContainer);

	TabContainer *GetTabContainer() const;

private:
	TabContainer *m_tabContainer;
};
