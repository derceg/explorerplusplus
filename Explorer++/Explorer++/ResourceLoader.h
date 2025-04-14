// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Icon.h"
#include <wil/resource.h>
#include <optional>
#include <string>

// The advantage of this interface is that it completely separates the task of loading a resource
// from the implementation of it.
class ResourceLoader
{
public:
	virtual ~ResourceLoader() = default;

	virtual std::wstring LoadString(UINT stringId) const = 0;
	virtual std::optional<std::wstring> MaybeLoadString(UINT stringId) const = 0;

	virtual wil::unique_hbitmap LoadBitmapFromPNGForDpi(Icon icon, int iconWidth, int iconHeight,
		int dpi) const = 0;
	virtual wil::unique_hbitmap LoadBitmapFromPNGAndScale(Icon icon, int iconWidth,
		int iconHeight) const = 0;
	virtual wil::unique_hicon LoadIconFromPNGForDpi(Icon icon, int iconWidth, int iconHeight,
		int dpi) const = 0;
	virtual wil::unique_hicon LoadIconFromPNGAndScale(Icon icon, int iconWidth,
		int iconHeight) const = 0;
};
