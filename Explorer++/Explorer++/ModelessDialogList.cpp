// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ModelessDialogList.h"
#include <ranges>

void ModelessDialogList::AddDialog(std::wstring id, HWND dialog)
{
	// There should only be a single instance of a modeless dialog open, so this call should always
	// result in an item being added.
	auto [itr, didInsert] = m_dialogMap.insert({ id, dialog });
	DCHECK(didInsert);
}

void ModelessDialogList::RemoveDialog(std::wstring id)
{
	auto numRemoved = m_dialogMap.erase(id);
	DCHECK_EQ(numRemoved, 1u);
}

concurrencpp::generator<HWND> ModelessDialogList::GetList() const
{
	for (auto dialog : m_dialogMap | std::views::values)
	{
		co_yield dialog;
	}
}

HWND ModelessDialogList::MaybeGetDialogById(std::wstring id) const
{
	auto itr = m_dialogMap.find(id);

	if (itr == m_dialogMap.end())
	{
		return nullptr;
	}

	return itr->second;
}
