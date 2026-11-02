// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabBackingToolbarView.h"
#include <commctrl.h>

TabBackingToolbarView::TabBackingToolbarView(ViewBase *parent, MenuHelpTextHost *menuHelpTextHost,
	const Config *config) :
	ToolbarView(parent->GetHWND(),
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS | TBSTYLE_LIST
			| TBSTYLE_TRANSPARENT | TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE,
		TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DOUBLEBUFFER, menuHelpTextHost, config)
{
}
