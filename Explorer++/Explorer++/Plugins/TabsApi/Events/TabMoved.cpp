// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Plugins/TabsApi/Events/TabMoved.h"
#include "TabContainer.h"
#include <sol/sol.hpp>

Plugins::TabMoved::TabMoved(TabContainer *tabContainer) : m_tabContainer(tabContainer)
{
}

boost::signals2::connection Plugins::TabMoved::connectObserver(sol::protected_function observer,
	sol::this_state state)
{
	UNREFERENCED_PARAMETER(state);

	return m_tabContainer->tabMovedSignal.AddObserver(
		[observer](const Tab &tab, int fromIndex, int toIndex)
		{ observer(tab.GetId(), fromIndex, toIndex); });
}
