// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Icon.h"
#include <wil/resource.h>
#include <gdiplus.h>

namespace IconResourceLoader
{
	wil::unique_hbitmap LoadBitmapFromPNGForDpi(Icon icon, int iconWidth, int iconHeight, int dpi);
	wil::unique_hbitmap LoadBitmapFromPNGAndScale(Icon icon, int iconWidth, int iconHeight);
	wil::unique_hicon LoadIconFromPNGForDpi(Icon icon, int iconWidth, int iconHeight, int dpi);
	wil::unique_hicon LoadIconFromPNGAndScale(Icon icon, int iconWidth, int iconHeight);
	std::unique_ptr<Gdiplus::Bitmap> LoadGdiplusBitmapFromPNGForDpi(Icon icon, int iconWidth, int iconHeight, int dpi);
	std::unique_ptr<Gdiplus::Bitmap> LoadGdiplusBitmapFromPNGAndScale(Icon icon, int iconWidth, int iconHeight);
}