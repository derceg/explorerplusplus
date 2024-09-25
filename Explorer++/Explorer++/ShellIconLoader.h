// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <shtypes.h>
#include <functional>

enum class ShellIconSize
{
	Small
};

using ShellIconUpdateCallback = std::function<void(wil::unique_hbitmap updatedIcon)>;

class ShellIconLoader
{
public:
	virtual ~ShellIconLoader() = default;

	virtual wil::unique_hbitmap LoadShellIcon(PCIDLIST_ABSOLUTE pidl, ShellIconSize size,
		ShellIconUpdateCallback updateCallback) = 0;
};
