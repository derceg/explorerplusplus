// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ListViewColumnModelFake.h"
#include "ListViewModel.h"

class ListViewItemFake;

class ListViewModelFake : public ListViewModel
{
public:
	ListViewModelFake(SortPolicy sortPolicy = SortPolicy::DoesntHaveDefault);

	ListViewColumnModel *GetColumnModel() override;
	const ListViewColumnModel *GetColumnModel() const override;

	ListViewItemFake *AddItem(const std::wstring &name = L"");

	using ListViewModel::RemoveAllItems;
	using ListViewModel::RemoveItem;

protected:
	std::weak_ordering CompareItems(const ListViewItem *first,
		const ListViewItem *second) const override;

private:
	ListViewColumnModelFake m_columnModel;
};
