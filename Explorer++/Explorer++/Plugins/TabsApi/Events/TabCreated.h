// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Plugins/Event.h"

class GlobalTabEventDispatcher;
class Tab;

namespace Plugins
{

class TabCreated : public Event
{
public:
	TabCreated(GlobalTabEventDispatcher *globalTabEventDispatcher);

protected:
	boost::signals2::connection connectObserver(sol::protected_function observer,
		sol::this_state state) override;

private:
	void onTabCreated(const Tab &tab, sol::protected_function observer);

	GlobalTabEventDispatcher *const m_globalTabEventDispatcher;
};

}
