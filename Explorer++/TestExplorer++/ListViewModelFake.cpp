// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ListViewModelFake.h"
#include "ListViewItemFake.h"

ListViewModelFake::ListViewModelFake(SortPolicy sortPolicy) : ListViewModel(sortPolicy)
{
}

ListViewColumnModel *ListViewModelFake::GetColumnModel()
{
	return &m_columnModel;
}

const ListViewColumnModel *ListViewModelFake::GetColumnModel() const
{
	return &m_columnModel;
}

ListViewItemFake *ListViewModelFake::AddItem(const std::wstring &name)
{
	auto item = std::make_unique<ListViewItemFake>(name);
	auto *rawItem = item.get();
	ListViewModel::AddItem(std::move(item));
	return rawItem;
}

std::weak_ordering ListViewModelFake::CompareItems(const ListViewItem *first,
	const ListViewItem *second) const
{
	const auto *firstFake = static_cast<const ListViewItemFake *>(first);
	const auto *secondFake = static_cast<const ListViewItemFake *>(second);

	if (GetSortColumnId() == ListViewColumnModelFake::COLUMN_NAME)
	{
		return firstFake->GetName() <=> secondFake->GetName();
	}

	// There is no explicitly defined ordering for anything but the name column. So, items can be
	// considered equivalent.
	return std::weak_ordering::equivalent;
}
