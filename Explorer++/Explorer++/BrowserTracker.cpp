// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserTracker.h"
#include "BrowserList.h"

BrowserTracker::BrowserTracker(BrowserList *browserList, BrowserWindow *browser) :
	m_browserList(browserList),
	m_browser(browser)
{
	m_browserList->AddBrowser(browser);
}

BrowserTracker::~BrowserTracker()
{
	m_browserList->RemoveBrowser(m_browser);
}
