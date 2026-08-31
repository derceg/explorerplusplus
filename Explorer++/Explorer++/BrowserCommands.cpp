// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserCommands.h"
#include "BrowserWindow.h"
#include "BrowserWindowFactory.h"
#include "MainRebarStorage.h"
#include "WindowStorage.h"

namespace BrowserCommands
{

void NewWindow(BrowserWindow *browser, BrowserWindowFactory *browserFactory,
	const std::vector<TabStorageData> &tabs)
{
	auto initialData = browser->GetStorageData();
	initialData.tabs = tabs;
	initialData.selectedTab = 0;

	constexpr int windowOffsetInPixels = 10;
	OffsetRect(&initialData.bounds, windowOffsetInPixels, windowOffsetInPixels);

	browserFactory->CreateBrowserWindow(initialData);
}

}
