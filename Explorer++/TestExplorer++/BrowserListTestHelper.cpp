// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "BrowserListTestHelper.h"
#include "BrowserList.h"

std::vector<BrowserWindow *> GetBrowserListAsVector(const BrowserList *browserList)
{
	std::vector<BrowserWindow *> browsers;

	for (auto *browser : browserList->GetList())
	{
		browsers.push_back(browser);
	}

	return browsers;
}
