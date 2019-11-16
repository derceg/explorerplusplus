// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Event.h"
#include "TabContainer.h"

namespace Plugins
{
	class TabUpdated : public Event
	{
	public:

		TabUpdated(TabContainer *tabContainer);

	protected:

		virtual boost::signals2::connection connectObserver(sol::protected_function observer, sol::this_state state);

	private:

		void onTabUpdated(sol::protected_function observer, sol::this_state state, const Tab &tab, Tab::PropertyType propertyType);

		TabContainer *m_tabContainer;
	};
}