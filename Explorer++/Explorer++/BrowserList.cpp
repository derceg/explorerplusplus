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

	browserAddedSignal.m_signal();
}

void BrowserList::RemoveBrowser(BrowserWindow *browser)
{
	auto numRemoved = m_browsers.erase(browser);
	DCHECK_EQ(numRemoved, 1u);

	browserRemovedSignal.m_signal();
}

concurrencpp::generator<BrowserWindow *> BrowserList::GetList() const
{
	for (const auto &browserData : m_browsers)
	{
		co_yield browserData.GetBrowser();
	}
}

BrowserWindow *BrowserList::GetLastActive() const
{
	if (m_browsers.empty())
	{
		return nullptr;
	}

	auto &activeTimeIndex = m_browsers.get<ByActiveTime>();
	return activeTimeIndex.begin()->GetBrowser();
}

void BrowserList::SetLastActive(BrowserWindow *browser)
{
	auto itr = m_browsers.find(browser);

	if (itr == m_browsers.end())
	{
		DCHECK(false);
		return;
	}

	m_browsers.modify(itr, [](auto &browserData) { browserData.UpdateLastActiveTime(); });
}

bool BrowserList::IsEmpty() const
{
	return m_browsers.empty();
}

BrowserList::BrowserData::BrowserData(BrowserWindow *browser) : m_browser(browser)
{
}

BrowserWindow *BrowserList::BrowserData::GetBrowser() const
{
	return m_browser;
}

BrowserList::BrowserData::Clock::time_point BrowserList::BrowserData::GetLastActiveTime() const
{
	return m_lastActiveTime;
}

void BrowserList::BrowserData::UpdateLastActiveTime()
{
	m_lastActiveTime = Clock::now();
}
