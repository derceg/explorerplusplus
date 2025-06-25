// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabRestorer.h"
#include "BrowserList.h"
#include "BrowserWindow.h"
#include "TabEvents.h"

TabRestorer::TabRestorer(TabEvents *tabEvents, const BrowserList *browserList) :
	m_browserList(browserList)
{
	m_connections.push_back(tabEvents->AddPreRemovalObserver(
		std::bind_front(&TabRestorer::OnTabPreRemoval, this), TabEventScope::Global()));
}

void TabRestorer::OnTabPreRemoval(const Tab &tab, int index)
{
	auto closedTab = std::make_unique<PreservedTab>(tab, index);
	m_closedTabs.insert(m_closedTabs.begin(), std::move(closedTab));
	m_itemsChangedSignal();
}

// TODO: This should use std::generator once C++23 support is available.
const std::list<std::unique_ptr<PreservedTab>> &TabRestorer::GetClosedTabs() const
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

bool TabRestorer::IsEmpty() const
{
	return m_closedTabs.empty();
}

Tab *TabRestorer::RestoreLastTab()
{
	if (m_closedTabs.empty())
	{
		return nullptr;
	}

	auto itr = m_closedTabs.begin();
	auto lastClosedTab = std::move(*itr);
	m_closedTabs.erase(itr);

	auto *restoredTab = RestoreTabIntoBrowser(lastClosedTab.get());
	m_itemsChangedSignal();

	return restoredTab;
}

Tab *TabRestorer::RestoreTabById(int id)
{
	auto itr = std::find_if(m_closedTabs.begin(), m_closedTabs.end(),
		[id](const std::unique_ptr<PreservedTab> &preservedTab) { return preservedTab->id == id; });

	if (itr == m_closedTabs.end())
	{
		return nullptr;
	}

	auto closedTab = std::move(*itr);
	m_closedTabs.erase(itr);

	auto *restoredTab = RestoreTabIntoBrowser(closedTab.get());
	m_itemsChangedSignal();

	return restoredTab;
}

Tab *TabRestorer::RestoreTabIntoBrowser(const PreservedTab *tab)
{
	auto *originalBrowser = m_browserList->MaybeGetById(tab->browserId);
	auto *targetBrowser = originalBrowser ? originalBrowser : m_browserList->GetLastActive();

	if (!targetBrowser)
	{
		return nullptr;
	}

	auto *restoredTab = targetBrowser->CreateTabFromPreservedTab(tab);
	targetBrowser->Activate();

	return restoredTab;
}

boost::signals2::connection TabRestorer::AddItemsChangedObserver(
	const ItemsChangedSignal::slot_type &observer)
{
	return m_itemsChangedSignal.connect(observer);
}
