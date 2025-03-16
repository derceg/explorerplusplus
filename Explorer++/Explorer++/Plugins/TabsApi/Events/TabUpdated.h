// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Plugins/Event.h"
#include "Tab.h"

class TabEvents;

namespace Plugins
{

class TabUpdated : public Event
{
public:
	TabUpdated(TabEvents *tabEvents);

protected:
	boost::signals2::connection connectObserver(sol::protected_function observer,
		sol::this_state state) override;

private:
	void onTabUpdated(sol::protected_function observer, sol::this_state state, const Tab &tab,
		Tab::PropertyType propertyType);

	TabEvents *const m_tabEvents;
};

}
