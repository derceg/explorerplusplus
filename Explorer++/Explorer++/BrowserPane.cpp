// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserPane.h"

BrowserPane::BrowserPane(TabContainer *tabContainer) : m_tabContainer(tabContainer)
{
}

TabContainer *BrowserPane::GetTabContainer() const
{
	return m_tabContainer;
}
