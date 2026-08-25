// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ListViewColumnModel.h"
#include "ListViewItem.h"
#include "ListViewModel.h"
#include "Literals.h"
#include "../Helper/BetterEnumsWrapper.h"
#include "../Helper/SignalWrapper.h"
#include <boost/signals2.hpp>
#include <vector>

class AsyncIconFetcher;
class SearchTabsModel;
class Tab;
class TabList;

BETTER_ENUM(SearchTabsColumn, int,
	TabName = 1,
	Path = 2
)

class SearchTabsColumnModel : public ListViewColumnModel
{
public:
	SearchTabsColumnModel();

	static ListViewColumnId SearchTabsColumnToColumnId(SearchTabsColumn column);
	static SearchTabsColumn ColumnIdToSearchTabsColumn(ListViewColumnId columnId);

private:
	static constexpr int DEFAULT_COLUMN_WIDTH = 180_px;

	static std::vector<ListViewColumn> BuildColumnSet();
};

class SearchTabsListViewItem : public ListViewItem
{
public:
	SearchTabsListViewItem(const Tab *tab, AsyncIconFetcher *iconFetcher);

	// ListViewItem
	std::wstring GetColumnText(ListViewColumnId columnId) const override;
	std::optional<int> GetIconIndex() const override;
	bool CanRename() const override;
	bool CanRemove() const override;
	bool IsFile() const override;

	const Tab *GetTab() const;

private:
	const Tab *const m_tab;
	AsyncIconFetcher *const m_iconFetcher;
};

class SearchTabsListViewModel : public ListViewModel
{
public:
	SearchTabsListViewModel(SearchTabsModel *searchTabsModel, const TabList *tabList,
		AsyncIconFetcher *iconFetcher);

	// ListViewModel
	ListViewColumnModel *GetColumnModel() override;
	const ListViewColumnModel *GetColumnModel() const override;

	const Tab *GetTabForItem(const ListViewItem *item) const;

	// Signals
	SignalWrapper<SearchTabsListViewModel, void()> itemsRefreshedSignal;

private:
	void UpdateItems();

	// ListViewModel
	std::weak_ordering CompareItems(const ListViewItem *first,
		const ListViewItem *second) const override;

	SearchTabsColumnModel m_columnModel;
	SearchTabsModel *const m_searchTabsModel;
	const TabList *const m_tabList;
	AsyncIconFetcher *const m_iconFetcher;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
