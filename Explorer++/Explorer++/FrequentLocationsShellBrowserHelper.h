// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/ShellBrowserHelper.h"

class FrequentLocationsModel;

class FrequentLocationsShellBrowserHelper :
	public ShellBrowserHelper<FrequentLocationsShellBrowserHelper>
{
public:
	FrequentLocationsShellBrowserHelper(ShellBrowser *shellBrowser, FrequentLocationsModel *model);

private:
	void OnNavigationCommitted(const NavigateParams &navigateParams);

	FrequentLocationsModel *const m_model;
};
