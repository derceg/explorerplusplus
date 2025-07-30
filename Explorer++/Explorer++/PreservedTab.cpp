// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PreservedTab.h"
#include "BrowserWindow.h"

PreservedTab::PreservedTab(const Tab &tab, int index) :
	id(tab.GetId()),
	browserId(tab.GetBrowser()->GetId()),
	index(index),
	useCustomName(tab.GetUseCustomName()),
	customName(tab.GetUseCustomName() ? tab.GetName() : std::wstring()),
	lockState(tab.GetLockState()),
	preservedShellBrowser(tab.GetShellBrowser())
{
}
