// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabUpdated.h"
#include "TabsApi.h"

Plugins::TabUpdated::TabUpdated(TabContainerInterface *tabContainer) :
	m_tabContainer(tabContainer)
{

}

Plugins::TabUpdated::~TabUpdated()
{

}

boost::signals2::connection Plugins::TabUpdated::connectObserver(sol::protected_function observer, sol::this_state state)
{
	return m_tabContainer->AddTabUpdatedObserver([this, observer, state] (const Tab &tab, Tab::PropertyType propertyType) {
		onTabUpdated(observer, state, tab, propertyType);
	});
}

void Plugins::TabUpdated::onTabUpdated(sol::protected_function observer, sol::this_state state,
	const Tab &tab, Tab::PropertyType propertyType)
{
	sol::state_view existingState = state;

	sol::table changeInfo = existingState.create_table();

	switch (propertyType)
	{
	case Tab::PropertyType::LOCKED:
		changeInfo["locked"] = tab.GetLocked();
		break;

	case Tab::PropertyType::ADDRESS_LOCKED:
		changeInfo["addressLocked"] = tab.GetAddressLocked();
		break;

	case Tab::PropertyType::NAME:
		changeInfo["name"] = tab.GetName();
		break;
	}

	TabsApi::Tab tabData(tab);

	observer(tab.GetId(), changeInfo, tabData);
}