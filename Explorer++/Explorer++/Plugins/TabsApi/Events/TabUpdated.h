// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Plugins/Event.h"
#include "Tab.h"

class TabContainer;

namespace Plugins
{
	class TabUpdated : public Event
	{
	public:

		TabUpdated(TabContainer *tabContainer);

	protected:

		boost::signals2::connection connectObserver(sol::protected_function observer, sol::this_state state) override;

	private:

		void onTabUpdated(sol::protected_function observer, sol::this_state state, const Tab &tab, Tab::PropertyType propertyType);

		TabContainer *m_tabContainer;
	};
}