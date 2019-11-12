// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Icon.h"
#include <wil/resource.h>
#include <gdiplus.h>

namespace IconResourceLoader
{
	wil::unique_hbitmap LoadBitmapFromPNGForDpi(Icon icon, int iconSize, int dpi);
	wil::unique_hicon LoadIconFromPNGForDpi(Icon icon, int iconSize, int dpi);
	std::unique_ptr<Gdiplus::Bitmap> LoadGdiplusBitmapFromPNGForDpi(Icon icon, int iconSize, int dpi);
}