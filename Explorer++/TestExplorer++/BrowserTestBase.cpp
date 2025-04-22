// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"

BrowserWindowFake *BrowserTestBase::AddBrowser()
{
	auto browser = std::make_unique<BrowserWindowFake>(&m_tabEvents, &m_navigationEvents);
	auto *rawBrowser = browser.get();
	m_browsers.push_back(std::move(browser));
	return rawBrowser;
}

void BrowserTestBase::RemoveBrowser(const BrowserWindowFake *browser)
{
	auto itr = std::ranges::find_if(m_browsers,
		[browser](const auto &currentBrowser) { return currentBrowser.get() == browser; });
	ASSERT_TRUE(itr != m_browsers.end());
	m_browsers.erase(itr);
}
