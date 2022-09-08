// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DrivesToolbarView.h"

DrivesToolbarView *DrivesToolbarView::Create(HWND parent)
{
	return new DrivesToolbarView(parent);
}

DrivesToolbarView::DrivesToolbarView(HWND parent) :
	ToolbarView(parent,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TBSTYLE_TOOLTIPS | TBSTYLE_LIST
			| TBSTYLE_TRANSPARENT | TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NORESIZE,
		TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_HIDECLIPPEDBUTTONS)
{
	SetupSmallShellImageList();
}
