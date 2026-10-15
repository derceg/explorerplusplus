// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/SignalWrapper.h"
#include <concurrencpp/concurrencpp.h>
#include <functional>
#include <string>
#include <vector>

namespace Plugins
{

class PluginMenuItemsRegistry
{
public:
	using ItemSelectedCallback = std::function<void()>;

	struct Item
	{
		UINT id;
		std::wstring text;
		ItemSelectedCallback selectedCallback;
	};

	UINT AddMenuItem(const std::wstring &text, ItemSelectedCallback selectedCallback);
	void RemoveMenuItem(UINT id);

	// TODO: This should use std::generator once C++23 support is available.
	concurrencpp::generator<const Item *> GetItems() const;

	// Signals
	SignalWrapper<PluginMenuItemsRegistry, void()> itemsChangedSignal;

private:
	// Note that this has no relation to the IDs used for the actual menu items. IDs generated using
	// this counter are used purely for the benefit of plugins (i.e. so that a plugin can identify a
	// particular menu item).
	UINT m_idCounter = 1;

	std::vector<Item> m_items;
};

}
