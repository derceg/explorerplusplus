// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabBarView.h"
#include "../Helper/DpiCompatibility.h"
#include <CommCtrl.h>

TabBarView::TabBarView(ViewBase *parent, const ThemeManager *themeManager, const Config *config,
	const ResourceLoader *resourceLoader) :
	TabView(parent,
		WS_CHILD | TCS_FOCUSNEVER | TCS_SINGLELINE | TCS_TOOLTIPS | WS_CLIPSIBLINGS
			| WS_CLIPCHILDREN,
		themeManager, config, resourceLoader),
	m_imageListManager(resourceLoader, DpiCompatibility::GetInstance().GetDpiForWindow(GetHWND()))
{
	SetImageList(m_imageListManager.GetImageList());
}

TabBarViewImageListManager *TabBarView::GetImageListManager()
{
	return &m_imageListManager;
}
