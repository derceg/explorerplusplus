// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellNavigator.h"

class ShellBrowserHelperBase;
class ShellNavigationController;

class ShellBrowser : public ShellNavigator
{
public:
	virtual ~ShellBrowser() = default;

	virtual ShellNavigationController *GetNavigationController() const = 0;
	virtual void AddHelper(std::unique_ptr<ShellBrowserHelperBase> helper) = 0;
};
