// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabCreated.h"
#include "TabsApi.h"

Plugins::TabCreated::TabCreated(TabContainerInterface *tabContainer) :
	m_tabContainer(tabContainer)
{

}

Plugins::TabCreated::~TabCreated()
{

}

boost::signals2::connection Plugins::TabCreated::connectObserver(sol::protected_function observer)
{
	return m_tabContainer->AddTabCreatedObserver([this, observer](int tabId, BOOL switchToNewTab) {
		UNREFERENCED_PARAMETER(switchToNewTab);

		onTabCreated(tabId, observer);
	});
}

void Plugins::TabCreated::onTabCreated(int tabId, sol::protected_function observer)
{
	auto tabInternal = m_tabContainer->GetTabOptional(tabId);

	if (!tabInternal)
	{
		return;
	}

	TabsApi::Tab tab(*tabInternal);
	observer(tab);
}