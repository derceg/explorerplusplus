// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "PreservedTab.h"
#include <boost/core/noncopyable.hpp>
#include <boost/signals2.hpp>

class TabContainer;

class TabRestorer : private boost::noncopyable
{
public:
	using ItemsChangedSignal = boost::signals2::signal<void()>;

	TabRestorer(TabContainer *tabContainer);

	const std::vector<std::unique_ptr<PreservedTab>> &GetClosedTabs() const;
	const PreservedTab *GetTabById(int id) const;
	void RestoreLastTab();
	void RestoreTabById(int id);

	boost::signals2::connection AddItemsChangedObserver(
		const ItemsChangedSignal::slot_type &observer);

private:
	void OnTabPreRemoval(const Tab &tab);

	TabContainer *m_tabContainer;
	std::vector<boost::signals2::scoped_connection> m_connections;

	std::vector<std::unique_ptr<PreservedTab>> m_closedTabs;

	ItemsChangedSignal m_itemsChangedSignal;
};
