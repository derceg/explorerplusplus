// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Plugins/TabsApi/Events/TabRemoved.h"
#include "Tab.h"
#include "TabEvents.h"
#include <sol/sol.hpp>

namespace Plugins
{

TabRemoved::TabRemoved(TabEvents *tabEvents) : m_tabEvents(tabEvents)
{
}

boost::signals2::connection TabRemoved::connectObserver(sol::protected_function observer,
	sol::this_state state)
{
	UNREFERENCED_PARAMETER(state);

	return m_tabEvents->AddRemovedObserver([observer](const Tab &tab) { observer(tab.GetId()); },
		TabEventScope::Global());
}

}
