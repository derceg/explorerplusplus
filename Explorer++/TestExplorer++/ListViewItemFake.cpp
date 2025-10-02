// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ListViewItemFake.h"
#include "ListViewColumnModelFake.h"

ListViewItemFake::ListViewItemFake(const std::wstring &name) : m_name(name)
{
}

std::wstring ListViewItemFake::GetColumnText(ListViewColumnId columnId) const
{
	if (columnId == ListViewColumnModelFake::COLUMN_NAME)
	{
		return m_name;
	}

	return L"";
}

std::optional<int> ListViewItemFake::GetIconIndex() const
{
	return std::nullopt;
}

bool ListViewItemFake::CanRename() const
{
	return false;
}

bool ListViewItemFake::CanRemove() const
{
	return false;
}

const std::wstring &ListViewItemFake::GetName() const
{
	return m_name;
}

void ListViewItemFake::SetName(const std::wstring &name)
{
	m_name = name;

	NotifyUpdated();
}
