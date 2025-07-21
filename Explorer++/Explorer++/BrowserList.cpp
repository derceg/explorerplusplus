// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserList.h"
#include "BrowserWindow.h"

void BrowserList::AddBrowser(BrowserWindow *browser)
{
	// Browsers should only be added a single time.
	auto [itr, didInsert] = m_browsers.insert(browser);
	DCHECK(didInsert);

	if (browser->IsActive())
	{
		SetLastActive(browser);
	}

	browserAddedSignal.m_signal(browser);
}

void BrowserList::WillRemoveBrowser(BrowserWindow *browser)
{
	// The browser should still exist in the list at this point.
	DCHECK(m_browsers.contains(browser));

	willRemoveBrowserSignal.m_signal(browser);
}

void BrowserList::RemoveBrowser(BrowserWindow *browser)
{
	auto numRemoved = m_browsers.erase(browser);
	DCHECK_EQ(numRemoved, 1u);

	browserRemovedSignal.m_signal(browser);
}

// TODO: This should use std::generator once C++23 support is available.
concurrencpp::generator<BrowserWindow *> BrowserList::GetList() const
{
	for (const auto &browserData : m_browsers)
	{
		co_yield browserData.GetMutableBrowser();
	}
}

BrowserWindow *BrowserList::MaybeGetById(int id) const
{
	auto &idIndex = m_browsers.get<ById>();
	auto itr = idIndex.find(id);

	if (itr == idIndex.end())
	{
		return nullptr;
	}

	return itr->GetMutableBrowser();
}

BrowserWindow *BrowserList::GetLastActive() const
{
	if (m_browsers.empty())
	{
		return nullptr;
	}

	auto &activeTimeIndex = m_browsers.get<ByActiveTime>();
	return activeTimeIndex.begin()->GetMutableBrowser();
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

size_t BrowserList::GetSize() const
{
	return m_browsers.size();
}

bool BrowserList::IsEmpty() const
{
	return m_browsers.empty();
}

BrowserList::BrowserData::BrowserData(BrowserWindow *browser) : m_browser(browser)
{
}

const BrowserWindow *BrowserList::BrowserData::GetBrowser() const
{
	return m_browser;
}

BrowserWindow *BrowserList::BrowserData::GetMutableBrowser() const
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

BrowserList::BrowserIdExtractor::result_type BrowserList::BrowserIdExtractor::operator()(
	const BrowserData &browserData) const
{
	return browserData.GetBrowser()->GetId();
}
