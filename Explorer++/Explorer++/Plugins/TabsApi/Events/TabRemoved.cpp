// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Plugins/TabsApi/Events/TabRemoved.h"
#include "GlobalTabEventDispatcher.h"
#include "Tab.h"
#include <sol/sol.hpp>

namespace Plugins
{

TabRemoved::TabRemoved(GlobalTabEventDispatcher *globalTabEventDispatcher) :
	m_globalTabEventDispatcher(globalTabEventDispatcher)
{
}

boost::signals2::connection TabRemoved::connectObserver(sol::protected_function observer,
	sol::this_state state)
{
	UNREFERENCED_PARAMETER(state);

	return m_globalTabEventDispatcher->AddRemovedObserver([observer](const Tab &tab)
		{ observer(tab.GetId()); }, TabEventScope::Global());
}

}
