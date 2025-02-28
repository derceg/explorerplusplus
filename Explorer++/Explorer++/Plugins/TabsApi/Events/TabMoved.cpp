// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Plugins/TabsApi/Events/TabMoved.h"
#include "GlobalTabEventDispatcher.h"
#include "Tab.h"
#include <sol/sol.hpp>

namespace Plugins
{

TabMoved::TabMoved(GlobalTabEventDispatcher *globalTabEventDispatcher) :
	m_globalTabEventDispatcher(globalTabEventDispatcher)
{
}

boost::signals2::connection TabMoved::connectObserver(sol::protected_function observer,
	sol::this_state state)
{
	UNREFERENCED_PARAMETER(state);

	return m_globalTabEventDispatcher->AddMovedObserver(
		[observer](const Tab &tab, int fromIndex, int toIndex)
		{ observer(tab.GetId(), fromIndex, toIndex); }, TabEventScope::Global());
}

}
