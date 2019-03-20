// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Event.h"
#include "TabContainerInterface.h"

namespace Plugins
{
	class TabRemoved : public Event
	{
	public:

		TabRemoved(TabContainerInterface *tabContainer);
		virtual ~TabRemoved();

	protected:

		virtual boost::signals2::connection connectObserver(sol::protected_function observer);

	private:

		TabContainerInterface *m_tabContainer;
	};
}