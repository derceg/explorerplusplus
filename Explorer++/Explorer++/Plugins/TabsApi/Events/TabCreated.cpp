// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Plugins/TabsApi/Events/TabCreated.h"
#include "Plugins/TabsApi/TabsApi.h"
#include "TabEvents.h"
#include <sol/sol.hpp>

namespace Plugins
{

TabCreated::TabCreated(TabEvents *tabEvents) : m_tabEvents(tabEvents)
{
}

boost::signals2::connection TabCreated::connectObserver(sol::protected_function observer,
	sol::this_state state)
{
	UNREFERENCED_PARAMETER(state);

	return m_tabEvents->AddCreatedObserver([this, observer](const Tab &tab)
		{ onTabCreated(tab, observer); }, TabEventScope::Global());
}

void TabCreated::onTabCreated(const Tab &tab, sol::protected_function observer)
{
	TabsApi::Tab apiTab(tab);
	observer(apiTab);
}

}
