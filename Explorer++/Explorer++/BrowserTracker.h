// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class BrowserList;
class BrowserWindow;

// This is designed to be held as a member variable within the class representing a top-level
// browser window. For example:
//
// BrowserTracker m_browserTracker;
//
// The browser window will be added to the BrowserList instance on construction and removed on
// destruction.
class BrowserTracker
{
public:
	BrowserTracker(BrowserList *browserList, BrowserWindow *browser);
	~BrowserTracker();

private:
	BrowserList *const m_browserList;
	BrowserWindow *const m_browser;
};
