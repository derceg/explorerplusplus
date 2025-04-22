// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabList.h"
#include "TabEvents.h"
#include <boost/range/iterator_range.hpp>

TabList::TabList(TabEvents *tabEvents)
{
	m_connections.push_back(
		tabEvents->AddCreatedObserver(std::bind_front(&TabList::OnTabCreated, this),
			TabEventScope::Global(), boost::signals2::at_front, SlotGroup::HighestPriority));
	m_connections.push_back(
		tabEvents->AddSelectedObserver(std::bind_front(&TabList::OnTabSelected, this),
			TabEventScope::Global(), boost::signals2::at_front, SlotGroup::HighestPriority));
	m_connections.push_back(
		tabEvents->AddRemovedObserver(std::bind_front(&TabList::OnTabRemoved, this),
			TabEventScope::Global(), boost::signals2::at_front, SlotGroup::HighestPriority));
}

Tab *TabList::GetById(int id) const
{
	auto &idIndex = m_tabs.get<ById>();
	auto itr = idIndex.find(id);
	CHECK(itr != idIndex.end());
	return itr->GetMutableTab();
}

// TODO: This should use std::generator once C++23 support is available.
concurrencpp::generator<Tab *> TabList::GetAll() const
{
	for (Tab *tab : m_tabs | std::views::transform(&TabList::ExtractTab))
	{
		co_yield tab;
	}
}

// TODO: This should use std::generator once C++23 support is available.
concurrencpp::generator<Tab *> TabList::GetAllByLastActiveTime() const
{
	for (Tab *tab : m_tabs.get<ByActiveTime>() | std::views::transform(&TabList::ExtractTab))
	{
		co_yield tab;
	}
}

// TODO: This should use std::generator once C++23 support is available.
concurrencpp::generator<Tab *> TabList::GetForBrowser(const BrowserWindow *browser) const
{
	for (Tab *tab : boost::make_iterator_range(m_tabs.get<ByBrowser>().equal_range(browser))
			| std::views::transform(&TabList::ExtractTab))
	{
		co_yield tab;
	}
}

Tab *TabList::ExtractTab(const TabData &tabData)
{
	return tabData.GetMutableTab();
}

void TabList::OnTabCreated(Tab &tab, bool selected)
{
	UNREFERENCED_PARAMETER(selected);

	auto [itr, didInsert] = m_tabs.insert(&tab);
	DCHECK(didInsert);
}

void TabList::OnTabSelected(const Tab &tab)
{
	auto itr = m_tabs.find(&tab);

	if (itr == m_tabs.end())
	{
		DCHECK(false);
		return;
	}

	m_tabs.modify(itr, [](auto &tabData) { tabData.UpdateLastActiveTime(); });
}

void TabList::OnTabRemoved(const Tab &tab)
{
	auto numRemoved = m_tabs.erase(&tab);
	DCHECK_EQ(numRemoved, 1u);
}

TabList::TabBrowserExtractor::result_type TabList::TabBrowserExtractor::operator()(
	const TabData &tabData) const
{
	return tabData.GetTab()->GetBrowser();
}

TabList::TabIdExtractor::result_type TabList::TabIdExtractor::operator()(
	const TabData &tabData) const
{
	return tabData.GetTab()->GetId();
}

TabList::TabData::TabData(Tab *tab) : m_tab(tab)
{
}

const Tab *TabList::TabData::GetTab() const
{
	return m_tab;
}

Tab *TabList::TabData::GetMutableTab() const
{
	return m_tab;
}

TabList::TabData::Clock::time_point TabList::TabData::GetLastActiveTime() const
{
	return m_lastActiveTime;
}

void TabList::TabData::UpdateLastActiveTime()
{
	m_lastActiveTime = Clock::now();
}
