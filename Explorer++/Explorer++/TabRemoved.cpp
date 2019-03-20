// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabRemoved.h"

Plugins::TabRemoved::TabRemoved(TabContainerInterface *tabContainer) :
	m_tabContainer(tabContainer)
{

}

Plugins::TabRemoved::~TabRemoved()
{

}

boost::signals2::connection Plugins::TabRemoved::connectObserver(sol::protected_function observer, sol::this_state state)
{
	UNREFERENCED_PARAMETER(state);

	return m_tabContainer->AddTabRemovedObserver(observer);
}