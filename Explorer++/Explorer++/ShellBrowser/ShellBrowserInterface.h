// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class ShellNavigationController;

class ShellBrowserInterface
{
public:
	virtual ~ShellBrowserInterface() = default;

	virtual ShellNavigationController *GetNavigationController() const = 0;
};
