// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabCreated.h"
#include "TabsApi.h"

Plugins::TabCreated::TabCreated(CTabContainer *tabContainer, TabContainerInterface *tabContainerInterface) :
	m_tabContainer(tabContainer),
	m_tabContainerInterface(tabContainerInterface)
{

}

Plugins::TabCreated::~TabCreated()
{

}

boost::signals2::connection Plugins::TabCreated::connectObserver(sol::protected_function observer, sol::this_state state)
{
	UNREFERENCED_PARAMETER(state);

	return m_tabContainerInterface->AddTabCreatedObserver([this, observer](int tabId, BOOL switchToNewTab) {
		UNREFERENCED_PARAMETER(switchToNewTab);

		onTabCreated(tabId, observer);
	});
}

void Plugins::TabCreated::onTabCreated(int tabId, sol::protected_function observer)
{
	const Tab &tabInternal = m_tabContainer->GetTab(tabId);

	TabsApi::Tab tab(tabInternal);
	observer(tab);
}