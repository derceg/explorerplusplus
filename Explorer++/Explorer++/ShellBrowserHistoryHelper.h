// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/ShellBrowserHelper.h"

class ShellBrowserHistoryHelper : public ShellBrowserHelper<ShellBrowserHistoryHelper>
{
public:
	ShellBrowserHistoryHelper(ShellBrowser *shellBrowser);

private:
	void OnNavigationCommitted(const NavigateParams &navigateParams);
};
