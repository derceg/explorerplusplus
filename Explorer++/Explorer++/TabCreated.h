// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Event.h"
#include "TabContainerInterface.h"

namespace Plugins
{
	class TabCreated : public Event
	{
	public:

		TabCreated(TabContainerInterface *tabContainer);
		virtual ~TabCreated();

	protected:

		virtual boost::signals2::connection connectObserver(sol::protected_function observer, sol::this_state state);

	private:

		void onTabCreated(int tabId, sol::protected_function observer);

		TabContainerInterface *m_tabContainer;
	};
}