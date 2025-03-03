// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Plugins/Event.h"

class Tab;
class TabEvents;

namespace Plugins
{

class TabCreated : public Event
{
public:
	TabCreated(TabEvents *tabEvents);

protected:
	boost::signals2::connection connectObserver(sol::protected_function observer,
		sol::this_state state) override;

private:
	void onTabCreated(const Tab &tab, sol::protected_function observer);

	TabEvents *const m_tabEvents;
};

}
