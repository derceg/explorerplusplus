// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Plugins/Event.h"

class GlobalTabEventDispatcher;

namespace Plugins
{

class TabMoved : public Event
{
public:
	TabMoved(GlobalTabEventDispatcher *globalTabEventDispatcher);

protected:
	boost::signals2::connection connectObserver(sol::protected_function observer,
		sol::this_state state) override;

private:
	GlobalTabEventDispatcher *const m_globalTabEventDispatcher;
};

}
