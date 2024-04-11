// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/ShellBrowserHelper.h"

class HistoryService;

class ShellBrowserHistoryHelper : public ShellBrowserHelper<ShellBrowserHistoryHelper>
{
public:
	ShellBrowserHistoryHelper(ShellBrowser *shellBrowser, HistoryService *historyService);

private:
	void OnNavigationCommitted(const NavigateParams &navigateParams);

	HistoryService *const m_historyService;
};
