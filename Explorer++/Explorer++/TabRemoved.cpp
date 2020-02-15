// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabRemoved.h"
#include "TabContainer.h"
#include "../ThirdParty/Sol/sol.hpp"

Plugins::TabRemoved::TabRemoved(TabContainer *tabContainer) :
	m_tabContainer(tabContainer)
{

}

boost::signals2::connection Plugins::TabRemoved::connectObserver(sol::protected_function observer, sol::this_state state)
{
	UNREFERENCED_PARAMETER(state);

	return m_tabContainer->tabRemovedSignal.AddObserver(observer);
}