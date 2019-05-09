// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabMoved.h"

Plugins::TabMoved::TabMoved(TabContainer *tabContainer) :
	m_tabContainer(tabContainer)
{

}

Plugins::TabMoved::~TabMoved()
{

}

boost::signals2::connection Plugins::TabMoved::connectObserver(sol::protected_function observer, sol::this_state state)
{
	UNREFERENCED_PARAMETER(state);

	return m_tabContainer->tabMovedSignal.AddObserver([observer] (const Tab &tab, int fromIndex, int toIndex) {
		observer(tab.GetId(), fromIndex, toIndex);
	});
}