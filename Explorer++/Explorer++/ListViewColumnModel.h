// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ListViewColumn.h"
#include "../Helper/SignalWrapper.h"
#include <concurrencpp/concurrencpp.h>
#include <functional>
#include <ranges>
#include <vector>

// Represents the columns that will be shown within a ListView.
class ListViewColumnModel
{
public:
	ListViewColumnModel(const std::vector<ListViewColumn> &columns,
		ListViewColumnId primaryColumnId);

	// This copies only the column state. Any observers aren't copied.
	ListViewColumnModel(const ListViewColumnModel &other);

	// This copies the column state and leaves observers on the current instance untouched.
	ListViewColumnModel &operator=(const ListViewColumnModel &other);

	virtual ~ListViewColumnModel() = default;

	concurrencpp::generator<const ListViewColumnId> GetAllColumnIds() const;
	concurrencpp::generator<const ListViewColumnId> GetVisibleColumnIds() const;
	int GetNumVisibleColumns() const;
	std::optional<int> MaybeGetColumnVisibleIndex(ListViewColumnId columnId) const;
	ListViewColumnId GetColumnIdAtVisibleIndex(int visibleIndex) const;
	bool IsColumnVisible(ListViewColumnId columnId) const;
	void SetColumnVisible(ListViewColumnId columnId, bool visible);

	ListViewColumnId GetPrimaryColumnId();
	ListViewColumn &GetColumnById(ListViewColumnId columnId);
	const ListViewColumn &GetColumnById(ListViewColumnId columnId) const;

	void MoveColumn(ListViewColumnId columnId, int newVisibleIndex);

	// Signals
	SignalWrapper<ListViewColumnModel, void(ListViewColumnId columnId, bool visible)>
		columnVisibilityChangedSignal;
	SignalWrapper<ListViewColumnModel, void(ListViewColumnId columnId, int newVisibleIndex)>
		columnMovedSignal;

private:
	auto GetVisibleColumnsView() const
	{
		return m_columns | std::views::filter(std::mem_fn(&ListViewColumn::visible));
	}

	int VisibleIndexToPhysicalIndex(int visibleIndex) const;

	std::vector<ListViewColumn> m_columns;
	ListViewColumnId m_primaryColumnId;
};
