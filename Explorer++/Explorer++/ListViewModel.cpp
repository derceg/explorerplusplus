// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ListViewModel.h"
#include "ListViewItem.h"
#include <algorithm>

ListViewModel::ListViewModel(SortPolicy sortPolicy) : m_sortPolicy(sortPolicy)
{
}

ListViewModel::~ListViewModel() = default;

void ListViewModel::AddItem(std::unique_ptr<ListViewItem> item)
{
	// The observer here doesn't need to be removed, since this class owns the item.
	std::ignore =
		item->AddUpdatedObserver(std::bind_front(&ListViewModel::OnItemUpdated, this, item.get()));

	int index = GetItemSortedIndex(item.get());
	auto itr = m_items.insert(m_items.begin() + index, std::move(item));

	itemAddedSignal.m_signal(itr->get(), index);
}

void ListViewModel::MaybeRepositionItem(ListViewItem *item)
{
	int originalIndex = GetItemIndex(item);

	auto itr = m_items.begin() + originalIndex;
	auto ownedItem = std::move(*itr);
	m_items.erase(itr);

	int updatedIndex = GetItemSortedIndex(item);
	m_items.insert(m_items.begin() + updatedIndex, std::move(ownedItem));

	if (updatedIndex != originalIndex)
	{
		itemMovedSignal.m_signal(item, updatedIndex);
	}
}

void ListViewModel::RemoveItem(ListViewItem *item)
{
	auto itr = std::ranges::find_if(m_items,
		[item](const auto &currentItem) { return currentItem.get() == item; });
	CHECK(itr != m_items.end());

	auto ownedItem = std::move(*itr);
	m_items.erase(itr);

	itemRemovedSignal.m_signal(ownedItem.get());
}

void ListViewModel::RemoveAllItems()
{
	m_items.clear();

	allItemsRemovedSignal.m_signal();
}

void ListViewModel::OnItemUpdated(ListViewItem *item)
{
	itemUpdatedSignal.m_signal(item);

	MaybeRepositionItem(item);
}

concurrencpp::generator<ListViewItem *> ListViewModel::GetItems()
{
	for (const auto &item : m_items)
	{
		co_yield item.get();
	}
}

int ListViewModel::GetNumItems() const
{
	return static_cast<int>(m_items.size());
}

int ListViewModel::GetItemIndex(const ListViewItem *item) const
{
	auto itr = std::ranges::find_if(m_items,
		[item](const auto &currentItem) { return currentItem.get() == item; });
	CHECK(itr != m_items.end());
	return static_cast<int>(std::distance(m_items.begin(), itr));
}

ListViewItem *ListViewModel::GetItemAtIndex(int index)
{
	return const_cast<ListViewItem *>(std::as_const(*this).GetItemAtIndex(index));
}

const ListViewItem *ListViewModel::GetItemAtIndex(int index) const
{
	CHECK(index >= 0 && index < GetNumItems());
	return m_items[index].get();
}

bool ListViewModel::HasDefaultSortOrder() const
{
	return m_sortPolicy == SortPolicy::HasDefault;
}

std::optional<ListViewColumnId> ListViewModel::GetSortColumnId() const
{
	return m_sortColumnId;
}

SortDirection ListViewModel::GetSortDirection() const
{
	return m_sortDirection;
}

void ListViewModel::SetSortDetails(std::optional<ListViewColumnId> columnId,
	SortDirection direction)
{
	if (columnId == m_sortColumnId && direction == m_sortDirection)
	{
		return;
	}

	m_sortColumnId = columnId;
	m_sortDirection = direction;

	SortItems();
}

void ListViewModel::SortItems()
{
	std::ranges::sort(m_items, [this](const auto &first, const auto &second)
		{ return CompareItemsWrapper(first.get(), second.get()); });

	sortOrderChangedSignal.m_signal();
}

int ListViewModel::GetItemSortedIndex(const ListViewItem *item) const
{
	DCHECK(!IsItemInSet(item));

	auto itr = std::ranges::upper_bound(m_items, item,
		std::bind_front(&ListViewModel::CompareItemsWrapper, this),
		[](const auto &currentItem) { return currentItem.get(); });
	auto index = std::distance(m_items.begin(), itr);
	return static_cast<int>(index);
}

bool ListViewModel::IsItemInSet(const ListViewItem *item) const
{
	auto itr = std::ranges::find_if(m_items,
		[item](const auto &currentItem) { return currentItem.get() == item; });
	return itr != m_items.end();
}

bool ListViewModel::CompareItemsWrapper(const ListViewItem *first, const ListViewItem *second) const
{
	auto cmp = CompareItems(first, second);
	return (GetSortDirection() == +SortDirection::Ascending) ? std::is_lt(cmp) : std::is_gt(cmp);
}
