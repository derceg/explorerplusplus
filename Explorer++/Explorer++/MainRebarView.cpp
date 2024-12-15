// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainRebarView.h"

MainRebarView *MainRebarView::Create(HWND parent)
{
	return new MainRebarView(parent);
}

MainRebarView::MainRebarView(HWND parent) :
	RebarView(parent,
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER | CCS_NODIVIDER
			| CCS_TOP | CCS_NOPARENTALIGN | RBS_BANDBORDERS | RBS_VARHEIGHT)
{
}
