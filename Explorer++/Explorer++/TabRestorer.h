// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "PreservedTab.h"
#include <boost/core/noncopyable.hpp>
#include <boost/signals2.hpp>
#include <list>
#include <vector>

class BrowserList;
class TabEvents;

class TabRestorer : private boost::noncopyable
{
public:
	using ItemsChangedSignal = boost::signals2::signal<void()>;

	TabRestorer(TabEvents *tabEvents, const BrowserList *browserList);

	const std::list<std::unique_ptr<PreservedTab>> &GetClosedTabs() const;
	const PreservedTab *GetTabById(int id) const;
	bool IsEmpty() const;
	void RestoreLastTab();
	void RestoreTabById(int id);

	boost::signals2::connection AddItemsChangedObserver(
		const ItemsChangedSignal::slot_type &observer);

private:
	void OnTabPreRemoval(const Tab &tab, int index);
	void RestoreTabIntoBrowser(const PreservedTab *tab);

	const BrowserList *const m_browserList;

	std::vector<boost::signals2::scoped_connection> m_connections;

	std::list<std::unique_ptr<PreservedTab>> m_closedTabs;
	ItemsChangedSignal m_itemsChangedSignal;
};
