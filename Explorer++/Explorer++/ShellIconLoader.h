// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "IconUpdateCallback.h"
#include "../Helper/IconSize.h"
#include <wil/resource.h>
#include <shtypes.h>

class ShellIconLoader
{
public:
	virtual ~ShellIconLoader() = default;

	virtual wil::unique_hbitmap LoadShellIcon(PCIDLIST_ABSOLUTE pidl, IconSize size,
		IconUpdateCallback updateCallback) = 0;
};
