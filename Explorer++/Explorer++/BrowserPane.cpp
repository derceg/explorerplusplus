// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserPane.h"

BrowserPane::BrowserPane(TabContainerImpl *tabContainerImpl) : m_tabContainerImpl(tabContainerImpl)
{
}

TabContainerImpl *BrowserPane::GetTabContainerImpl() const
{
	return m_tabContainerImpl;
}
