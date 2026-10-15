// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Plugins/PluginMenuItemsRegistry.h"

namespace Plugins
{

UINT PluginMenuItemsRegistry::AddMenuItem(const std::wstring &text,
	ItemSelectedCallback selectedCallback)
{
	UINT id = m_idCounter++;
	m_items.emplace_back(id, text, std::move(selectedCallback));

	itemsChangedSignal.m_signal();

	return id;
}

void PluginMenuItemsRegistry::RemoveMenuItem(UINT id)
{
	auto numErased = std::erase_if(m_items, [id](const auto &item) { return item.id == id; });
	DCHECK_EQ(numErased, 1u);

	itemsChangedSignal.m_signal();
}

concurrencpp::generator<const PluginMenuItemsRegistry::Item *> PluginMenuItemsRegistry::GetItems()
	const
{
	for (const auto &item : m_items)
	{
		co_yield &item;
	}
}

}
