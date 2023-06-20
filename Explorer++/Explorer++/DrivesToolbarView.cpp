// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DrivesToolbarView.h"

DrivesToolbarView *DrivesToolbarView::Create(HWND parent, const Config *config)
{
	return new DrivesToolbarView(parent, config);
}

DrivesToolbarView::DrivesToolbarView(HWND parent, const Config *config) :
	ToolbarView(parent,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TBSTYLE_TOOLTIPS | TBSTYLE_LIST
			| TBSTYLE_TRANSPARENT | TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NORESIZE,
		TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_HIDECLIPPEDBUTTONS, config)
{
	SetupSmallShellImageList();
}
