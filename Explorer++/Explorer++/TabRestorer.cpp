// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabRestorer.h"
#include "TabContainer.h"

TabRestorer::TabRestorer(TabContainer *tabContainer) : m_tabContainer(tabContainer)
{
	m_connections.push_back(m_tabContainer->tabPreRemovalSignal.AddObserver(
		std::bind_front(&TabRestorer::OnTabPreRemoval, this)));
}

void TabRestorer::OnTabPreRemoval(const Tab &tab)
{
	auto closedTab = std::make_unique<PreservedTab>(tab, m_tabContainer->GetTabIndex(tab));
	m_closedTabs.insert(m_closedTabs.begin(), std::move(closedTab));
	m_itemsChangedSignal();
}

const std::vector<std::unique_ptr<PreservedTab>> &TabRestorer::GetClosedTabs() const
{
	return m_closedTabs;
}

const PreservedTab *TabRestorer::GetTabById(int id) const
{
	auto itr = std::find_if(m_closedTabs.begin(), m_closedTabs.end(),
		[id](const std::unique_ptr<PreservedTab> &preservedTab) { return preservedTab->id == id; });

	if (itr == m_closedTabs.end())
	{
		return nullptr;
	}

	return itr->get();
}

void TabRestorer::RestoreLastTab()
{
	if (m_closedTabs.empty())
	{
		return;
	}

	auto itr = m_closedTabs.begin();

	auto lastClosedTab = itr->get();
	m_tabContainer->CreateNewTab(*lastClosedTab);
	m_closedTabs.erase(itr);
	m_itemsChangedSignal();
}

void TabRestorer::RestoreTabById(int id)
{
	auto itr = std::find_if(m_closedTabs.begin(), m_closedTabs.end(),
		[id](const std::unique_ptr<PreservedTab> &preservedTab) { return preservedTab->id == id; });

	if (itr == m_closedTabs.end())
	{
		return;
	}

	auto closedTab = itr->get();
	m_tabContainer->CreateNewTab(*closedTab);
	m_closedTabs.erase(itr);
	m_itemsChangedSignal();
}

boost::signals2::connection TabRestorer::AddItemsChangedObserver(
	const ItemsChangedSignal::slot_type &observer)
{
	return m_itemsChangedSignal.connect(observer);
}
