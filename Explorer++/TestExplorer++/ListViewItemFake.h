// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ListViewItem.h"
#include <string>

class ListViewItemFake : public ListViewItem
{
public:
	ListViewItemFake(const std::wstring &name);

	std::wstring GetColumnText(ListViewColumnId columnId) const override;
	std::optional<int> GetIconIndex() const override;
	bool CanRename() const override;
	bool CanRemove() const override;

	const std::wstring &GetName() const;
	void SetName(const std::wstring &name);

private:
	std::wstring m_name;
};
