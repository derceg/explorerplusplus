// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TerminalMessageRouter.h"
#include "BrowserWindow.h"
#include "Tab.h"
#include "TabContainer.h"

TerminalMessageRouter::Result TerminalMessageRouter::Process(const MSG *msg, BrowserWindow *browser)
{
	auto &selectedTab = browser->GetActiveTabContainer()->GetSelectedTab();
	return selectedTab.ProcessContentMessage(msg);
}
