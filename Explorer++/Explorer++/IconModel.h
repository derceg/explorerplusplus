// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "IconUpdateCallback.h"
#include <wil/com.h>
#include <wincodec.h>

// Represents an icon that can ultimately be loaded at a specific DPI. This allows a particular icon
// to be specified, without having to actually load it or handle DPI scaling.
class IconModel
{
public:
	virtual ~IconModel() = default;

	virtual wil::com_ptr_nothrow<IWICBitmapSource> GetBitmap(UINT dpi,
		IconUpdateCallback updateCallback) const = 0;
};
