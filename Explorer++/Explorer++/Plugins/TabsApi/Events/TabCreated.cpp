// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Plugins/TabsApi/Events/TabCreated.h"
#include "GlobalTabEventDispatcher.h"
#include "Plugins/TabsApi/TabsApi.h"
#include <sol/sol.hpp>

namespace Plugins
{

TabCreated::TabCreated(GlobalTabEventDispatcher *globalTabEventDispatcher) :
	m_globalTabEventDispatcher(globalTabEventDispatcher)
{
}

boost::signals2::connection TabCreated::connectObserver(sol::protected_function observer,
	sol::this_state state)
{
	UNREFERENCED_PARAMETER(state);

	return m_globalTabEventDispatcher->AddCreatedObserver(
		[this, observer](const Tab &tab, bool selected)
		{
			UNREFERENCED_PARAMETER(selected);

			onTabCreated(tab, observer);
		},
		TabEventScope::Global());
}

void TabCreated::onTabCreated(const Tab &tab, sol::protected_function observer)
{
	TabsApi::Tab apiTab(tab);
	observer(apiTab);
}

}
