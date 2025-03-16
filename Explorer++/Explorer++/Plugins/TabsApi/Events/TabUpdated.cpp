// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Plugins/TabsApi/Events/TabUpdated.h"
#include "Plugins/TabsApi/TabsApi.h"
#include "TabEvents.h"
#include <sol/sol.hpp>

namespace Plugins
{

TabUpdated::TabUpdated(TabEvents *tabEvents) : m_tabEvents(tabEvents)
{
}

boost::signals2::connection TabUpdated::connectObserver(sol::protected_function observer,
	sol::this_state state)
{
	return m_tabEvents->AddUpdatedObserver(
		[this, observer, state](const Tab &tab, Tab::PropertyType propertyType)
		{ onTabUpdated(observer, state, tab, propertyType); }, TabEventScope::Global());
}

void TabUpdated::onTabUpdated(sol::protected_function observer, sol::this_state state,
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

}
