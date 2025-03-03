// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Plugins/Event.h"

class TabEvents;

namespace Plugins
{

class TabMoved : public Event
{
public:
	TabMoved(TabEvents *tabEvents);

protected:
	boost::signals2::connection connectObserver(sol::protected_function observer,
		sol::this_state state) override;

private:
	TabEvents *const m_tabEvents;
};

}
