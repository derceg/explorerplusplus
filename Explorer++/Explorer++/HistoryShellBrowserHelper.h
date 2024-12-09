// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/ShellBrowserHelper.h"

class HistoryModel;

class HistoryShellBrowserHelper : public ShellBrowserHelper<HistoryShellBrowserHelper>
{
public:
	HistoryShellBrowserHelper(ShellBrowser *shellBrowser, HistoryModel *historyModel);

private:
	void OnNavigationCommitted(const NavigateParams &navigateParams);

	HistoryModel *const m_historyModel;
};
