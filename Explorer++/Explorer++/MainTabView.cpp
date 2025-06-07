// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainTabView.h"
#include <CommCtrl.h>

MainTabView *MainTabView::Create(HWND parent, const Config *config)
{
	return new MainTabView(parent, config);
}

MainTabView::MainTabView(HWND parent, const Config *config) :
	TabView(parent,
		WS_VISIBLE | WS_CHILD | TCS_FOCUSNEVER | TCS_SINGLELINE | TCS_TOOLTIPS | WS_CLIPSIBLINGS
			| WS_CLIPCHILDREN,
		config)
{
}
