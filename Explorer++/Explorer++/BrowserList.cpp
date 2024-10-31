// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserList.h"

void BrowserList::AddBrowser(BrowserWindow *browser)
{
	// Browsers should only be added a single time.
	auto [itr, didInsert] = m_browsers.insert(browser);
	DCHECK(didInsert);
}

void BrowserList::RemoveBrowser(BrowserWindow *browser)
{
	auto numRemoved = m_browsers.erase(browser);
	DCHECK_EQ(numRemoved, 1u);
}

concurrencpp::generator<BrowserWindow *> BrowserList::GetList() const
{
	for (auto *browser : m_browsers)
	{
		co_yield browser;
	}
}
