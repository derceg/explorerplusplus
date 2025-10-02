// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ListViewColumnModel.h"
#include "../Helper/Helper.h"
#include <algorithm>

ListViewColumnModel::ListViewColumnModel(const std::vector<ListViewColumn> &columns,
	ListViewColumnId primaryColumnId) :
	m_columns(columns),
	m_primaryColumnId(primaryColumnId)
{
	// At the very least, there needs to be one column (the primary column) and that column should
	// be visible.
	auto itr = std::ranges::find_if(m_columns,
		[primaryColumnId](const auto &column) { return column.id == primaryColumnId; });
	CHECK(itr != m_columns.end());
	CHECK(itr->visible);
}

ListViewColumnModel::ListViewColumnModel(const ListViewColumnModel &other) :
	m_columns(other.m_columns),
	m_primaryColumnId(other.m_primaryColumnId)
{
}

ListViewColumnModel &ListViewColumnModel::operator=(const ListViewColumnModel &other)
{
	m_columns = other.m_columns;
	m_primaryColumnId = other.m_primaryColumnId;
	return *this;
}

concurrencpp::generator<const ListViewColumnId> ListViewColumnModel::GetAllColumnIds() const
{
	for (const auto &column : m_columns)
	{
		co_yield column.id;
	}
}

concurrencpp::generator<const ListViewColumnId> ListViewColumnModel::GetVisibleColumnIds() const
{
	for (const auto &column : GetVisibleColumnsView())
	{
		co_yield column.id;
	}
}

int ListViewColumnModel::GetNumVisibleColumns() const
{
	return static_cast<int>(
		std::ranges::count_if(m_columns, std::mem_fn(&ListViewColumn::visible)));
}

std::optional<int> ListViewColumnModel::MaybeGetColumnVisibleIndex(ListViewColumnId columnId) const
{
	auto visibleView = GetVisibleColumnsView();
	auto itr = std::ranges::find_if(visibleView,
		[columnId](const auto &column) { return column.id == columnId; });

	if (itr == visibleView.end())
	{
		return std::nullopt;
	}

	return static_cast<int>(std::distance(visibleView.begin(), itr));
}

ListViewColumnId ListViewColumnModel::GetColumnIdAtVisibleIndex(int visibleIndex) const
{
	auto visibleView = GetVisibleColumnsView();
	auto itr = std::ranges::next(visibleView.begin(), visibleIndex, visibleView.end());
	CHECK(itr != visibleView.end());
	return itr->id;
}

bool ListViewColumnModel::IsColumnVisible(ListViewColumnId columnId) const
{
	const auto &column = GetColumnById(columnId);
	return column.visible;
}

void ListViewColumnModel::SetColumnVisible(ListViewColumnId columnId, bool visible)
{
	if (columnId == m_primaryColumnId)
	{
		// The primary column is always visible.
		return;
	}

	auto &column = GetColumnById(columnId);

	if (column.visible == visible)
	{
		return;
	}

	column.visible = visible;

	columnVisibilityChangedSignal.m_signal(column.id, visible);
}

ListViewColumnId ListViewColumnModel::GetPrimaryColumnId()
{
	return m_primaryColumnId;
}

ListViewColumn &ListViewColumnModel::GetColumnById(ListViewColumnId columnId)
{
	return const_cast<ListViewColumn &>(std::as_const(*this).GetColumnById(columnId));
}

const ListViewColumn &ListViewColumnModel::GetColumnById(ListViewColumnId columnId) const
{
	auto itr = std::ranges::find_if(m_columns,
		[columnId](const auto &column) { return column.id == columnId; });
	CHECK(itr != m_columns.end());
	return *itr;
}

void ListViewColumnModel::MoveColumn(ListViewColumnId columnId, int newVisibleIndex)
{
	auto itr = std::ranges::find_if(m_columns,
		[columnId](const auto &column) { return column.id == columnId; });
	CHECK(itr != m_columns.end());

	MoveVectorItem(m_columns, std::distance(m_columns.begin(), itr),
		VisibleIndexToPhysicalIndex(newVisibleIndex));

	columnMovedSignal.m_signal(columnId, newVisibleIndex);
}

int ListViewColumnModel::VisibleIndexToPhysicalIndex(int visibleIndex) const
{
	int currentVisibleIndex = 0;

	auto itr = std::ranges::find_if(m_columns,
		[&currentVisibleIndex, visibleIndex](const auto &column)
		{
			if (!column.visible)
			{
				return false;
			}

			return currentVisibleIndex++ == visibleIndex;
		});
	CHECK(itr != m_columns.end());

	return static_cast<int>(std::distance(m_columns.begin(), itr));
}
