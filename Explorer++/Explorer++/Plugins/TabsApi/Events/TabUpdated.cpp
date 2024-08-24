// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Plugins/TabsApi/Events/TabUpdated.h"
#include "Plugins/TabsApi/TabsApi.h"
#include "TabContainer.h"
#include <sol/sol.hpp>

Plugins::TabUpdated::TabUpdated(TabContainer *tabContainer) : m_tabContainer(tabContainer)
{
}

boost::signals2::connection Plugins::TabUpdated::connectObserver(sol::protected_function observer,
	sol::this_state state)
{
	return m_tabContainer->tabUpdatedSignal.AddObserver(
		[this, observer, state](const Tab &tab, Tab::PropertyType propertyType)
		{ onTabUpdated(observer, state, tab, propertyType); });
}

void Plugins::TabUpdated::onTabUpdated(sol::protected_function observer, sol::this_state state,
	const Tab &tab, Tab::PropertyType propertyType)
{
	sol::state_view existingState = state;

	sol::table changeInfo = existingState.create_table();

	switch (propertyType)
	{
	case Tab::PropertyType::Name:
		changeInfo["name"] = tab.GetName();
		break;

	case Tab::PropertyType::LockState:
		changeInfo["lockState"] = tab.GetLockState();
		break;
	}

	TabsApi::Tab tabData(tab);

	observer(tab.GetId(), changeInfo, tabData);
}
