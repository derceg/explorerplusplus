// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "IconUpdateCallback.h"
#include "../Helper/IconSize.h"
#include <wil/com.h>
#include <shtypes.h>
#include <wincodec.h>
#include <stop_token>

class ShellIconLoader
{
public:
	virtual ~ShellIconLoader() = default;

	virtual wil::com_ptr_nothrow<IWICBitmapSource> LoadShellIcon(PCIDLIST_ABSOLUTE pidl,
		IconSize size, IconUpdateCallback updateCallback, std::stop_token stopToken) = 0;
};
