// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "IconUpdateCallback.h"
#include <wil/resource.h>
#include <shtypes.h>

enum class ShellIconSize
{
	Small
};

class ShellIconLoader
{
public:
	virtual ~ShellIconLoader() = default;

	virtual wil::unique_hbitmap LoadShellIcon(PCIDLIST_ABSOLUTE pidl, ShellIconSize size,
		IconUpdateCallback updateCallback) = 0;
};
