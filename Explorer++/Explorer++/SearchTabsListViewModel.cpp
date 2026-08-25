// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SearchTabsListViewModel.h"
#include "AsyncIconFetcher.h"
#include "MainResource.h"
#include "SearchTabsModel.h"
#include "ShellBrowser/ShellBrowser.h"
#include "Tab.h"
#include "TabList.h"
#include "../Helper/Helper.h"

SearchTabsColumnModel::SearchTabsColumnModel() :
	ListViewColumnModel(BuildColumnSet(), SearchTabsColumnToColumnId(SearchTabsColumn::TabName),
		LayoutChanges::Disallowed)
{
}

std::vector<ListViewColumn> SearchTabsColumnModel::BuildColumnSet()
{
	return {
		{ SearchTabsColumnToColumnId(SearchTabsColumn::TabName), IDS_SEARCH_TABS_COLUMN_TAB_NAME,
			DEFAULT_COLUMN_WIDTH, true },
		{ SearchTabsColumnToColumnId(SearchTabsColumn::Path), IDS_SEARCH_TABS_COLUMN_PATH,
			DEFAULT_COLUMN_WIDTH, true },
	};
}

ListViewColumnId SearchTabsColumnModel::SearchTabsColumnToColumnId(SearchTabsColumn column)
{
	return ListViewColumnId(static_cast<int>(column));
}

SearchTabsColumn SearchTabsColumnModel::ColumnIdToSearchTabsColumn(ListViewColumnId columnId)
{
	auto column = SearchTabsColumn::_from_integral_nothrow(columnId.value);
	CHECK(column);
	return *column;
}

SearchTabsListViewItem::SearchTabsListViewItem(const Tab *tab, AsyncIconFetcher *iconFetcher) :
	m_tab(tab),
	m_iconFetcher(iconFetcher)
{
}

std::wstring SearchTabsListViewItem::GetColumnText(ListViewColumnId columnId) const
{
	switch (SearchTabsColumnModel::ColumnIdToSearchTabsColumn(columnId))
	{
	case SearchTabsColumn::TabName:
		return m_tab->GetName();

	case SearchTabsColumn::Path:
		return m_tab->GetShellBrowser()->GetDirectoryPath();
	}

	LOG(FATAL) << "Invalid SearchTabsColumn value";
}

std::optional<int> SearchTabsListViewItem::GetIconIndex() const
{
	return m_iconFetcher->GetCachedIconIndexOrDefault(
		m_tab->GetShellBrowser()->GetDirectory().Raw());
}

bool SearchTabsListViewItem::CanRename() const
{
	return false;
}

bool SearchTabsListViewItem::CanRemove() const
{
	return false;
}

bool SearchTabsListViewItem::IsFile() const
{
	return false;
}

const Tab *SearchTabsListViewItem::GetTab() const
{
	return m_tab;
}

SearchTabsListViewModel::SearchTabsListViewModel(SearchTabsModel *searchTabsModel,
	const TabList *tabList, AsyncIconFetcher *iconFetcher) :
	ListViewModel(SortPolicy::HasDefault),
	m_searchTabsModel(searchTabsModel),
	m_tabList(tabList),
	m_iconFetcher(iconFetcher)
{
	m_connections.push_back(searchTabsModel->updatedSignal.AddObserver(
		std::bind_front(&SearchTabsListViewModel::UpdateItems, this)));

	UpdateItems();
}

void SearchTabsListViewModel::UpdateItems()
{
	auto batchUpdates = BeginBatchUpdates();

	RemoveAllItems();

	for (const auto *tab : m_searchTabsModel->GetResults())
	{
		AddItem(std::make_unique<SearchTabsListViewItem>(tab, m_iconFetcher));
	}

	itemsRefreshedSignal.m_signal();
}

ListViewColumnModel *SearchTabsListViewModel::GetColumnModel()
{
	return &m_columnModel;
}

const ListViewColumnModel *SearchTabsListViewModel::GetColumnModel() const
{
	return &m_columnModel;
}

const Tab *SearchTabsListViewModel::GetTabForItem(const ListViewItem *item) const
{
	return static_cast<const SearchTabsListViewItem *>(item)->GetTab();
}

std::weak_ordering SearchTabsListViewModel::CompareItems(const ListViewItem *first,
	const ListViewItem *second) const
{
	auto sortColumnId = GetSortColumnId();

	if (sortColumnId)
	{
		switch (SearchTabsColumnModel::ColumnIdToSearchTabsColumn(*sortColumnId))
		{
		case SearchTabsColumn::TabName:
		case SearchTabsColumn::Path:
			return StrCmpLogicalW(first->GetColumnText(*sortColumnId).c_str(),
					   second->GetColumnText(*sortColumnId).c_str())
				<=> 0;
		}
	}

	// The ordering is reversed here, since more recently activated tabs (i.e. those with a greater
	// activation point) should appear first.
	const auto *tab1 = GetTabForItem(first);
	const auto *tab2 = GetTabForItem(second);
	return ReverseOrdering(
		m_tabList->GetTabLastActivationPoint(tab1) <=> m_tabList->GetTabLastActivationPoint(tab2));
}
