// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainTabView.h"
#include "../Helper/DpiCompatibility.h"
#include <CommCtrl.h>

MainTabView *MainTabView::Create(HWND parent, const Config *config,
	const ResourceLoader *resourceLoader)
{
	return new MainTabView(parent, config, resourceLoader);
}

MainTabView::MainTabView(HWND parent, const Config *config, const ResourceLoader *resourceLoader) :
	TabView(parent,
		WS_VISIBLE | WS_CHILD | TCS_FOCUSNEVER | TCS_SINGLELINE | TCS_TOOLTIPS | WS_CLIPSIBLINGS
			| WS_CLIPCHILDREN,
		config),
	m_imageListManager(resourceLoader, DpiCompatibility::GetInstance().GetDpiForWindow(GetHWND()))
{
	SetImageList(m_imageListManager.GetImageList());
}

MainTabViewImageListManager *MainTabView::GetImageListManager()
{
	return &m_imageListManager;
}
