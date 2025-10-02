// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ListViewColumn.h"
#include "../Helper/SignalWrapper.h"
#include "../Helper/SortDirection.h"
#include <concurrencpp/concurrencpp.h>
#include <compare>
#include <memory>
#include <optional>
#include <vector>

class ListViewColumnModel;
class ListViewItem;

// Represents the set of items displayed in a ListView.
class ListViewModel
{
public:
	enum class SortPolicy
	{
		// Indicates that the items have a default ordering. That is, even if a sort mode isn't
		// explicitly applied, the items still have a natural order.
		HasDefault,

		// Indicates there is no default ordering.
		DoesntHaveDefault
	};

	virtual ~ListViewModel();

	virtual ListViewColumnModel *GetColumnModel() = 0;
	virtual const ListViewColumnModel *GetColumnModel() const = 0;

	concurrencpp::generator<ListViewItem *> GetItems();
	int GetNumItems() const;

	int GetItemIndex(const ListViewItem *item) const;
	ListViewItem *GetItemAtIndex(int index);
	const ListViewItem *GetItemAtIndex(int index) const;

	bool HasDefaultSortOrder() const;
	std::optional<ListViewColumnId> GetSortColumnId() const;
	SortDirection GetSortDirection() const;
	void SetSortDetails(std::optional<ListViewColumnId> columnId, SortDirection direction);

	// Signals
	SignalWrapper<ListViewModel, void(ListViewItem *item, int index)> itemAddedSignal;
	SignalWrapper<ListViewModel, void(ListViewItem *item)> itemUpdatedSignal;
	SignalWrapper<ListViewModel, void(ListViewItem *item, int newIndex)> itemMovedSignal;
	SignalWrapper<ListViewModel, void(const ListViewItem *item)> itemRemovedSignal;
	SignalWrapper<ListViewModel, void()> allItemsRemovedSignal;
	SignalWrapper<ListViewModel, void()> sortOrderChangedSignal;

protected:
	ListViewModel(SortPolicy sortPolicy);

	void AddItem(std::unique_ptr<ListViewItem> item);

	// Called when an item's sorted position may have changed.
	void MaybeRepositionItem(ListViewItem *item);

	void RemoveItem(ListViewItem *item);
	void RemoveAllItems();

	virtual std::weak_ordering CompareItems(const ListViewItem *first,
		const ListViewItem *second) const = 0;

private:
	void OnItemUpdated(ListViewItem *item);

	void SortItems();
	int GetItemSortedIndex(const ListViewItem *item) const;
	bool IsItemInSet(const ListViewItem *item) const;
	bool CompareItemsWrapper(const ListViewItem *first, const ListViewItem *second) const;

	std::vector<std::unique_ptr<ListViewItem>> m_items;
	const SortPolicy m_sortPolicy;

	// If this is empty, it means that there is no explicit sort order. Items should either revert
	// back to the default ordering (if applicable), or be left in an unsorted state.
	std::optional<ListViewColumnId> m_sortColumnId;

	SortDirection m_sortDirection = SortDirection::Ascending;
};
