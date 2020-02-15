// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Event.h"

class TabContainer;

namespace Plugins
{
	class TabRemoved : public Event
	{
	public:

		TabRemoved(TabContainer *tabContainer);

	protected:

		boost::signals2::connection connectObserver(sol::protected_function observer, sol::this_state state) override;

	private:

		TabContainer *m_tabContainer;
	};
}