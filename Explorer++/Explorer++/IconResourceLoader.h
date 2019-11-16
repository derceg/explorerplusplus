// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BetterEnumsWrapper.h"
#include "Icon.h"
#include <wil/resource.h>
#include <gdiplus.h>

BETTER_ENUM(IconTheme, int,
	Color = 0,
	Windows10 = 1
)

class IconResourceLoader
{
public:

	IconResourceLoader(IconTheme iconTheme);

	wil::unique_hbitmap LoadBitmapFromPNGForDpi(Icon icon, int iconWidth, int iconHeight, int dpi) const;
	wil::unique_hbitmap LoadBitmapFromPNGAndScale(Icon icon, int iconWidth, int iconHeight) const;
	wil::unique_hicon LoadIconFromPNGForDpi(Icon icon, int iconWidth, int iconHeight, int dpi) const;
	wil::unique_hicon LoadIconFromPNGAndScale(Icon icon, int iconWidth, int iconHeight) const;
	std::unique_ptr<Gdiplus::Bitmap> LoadGdiplusBitmapFromPNGForDpi(Icon icon, int iconWidth, int iconHeight, int dpi) const;
	std::unique_ptr<Gdiplus::Bitmap> LoadGdiplusBitmapFromPNGAndScale(Icon icon, int iconWidth, int iconHeight) const;

private:

	wil::unique_hbitmap RetrieveBitmapFromGdiplusBitmap(Gdiplus::Bitmap *gdiplusBitmap) const;
	wil::unique_hicon RetrieveIconFromGdiplusBitmap(Gdiplus::Bitmap *gdiplusBitmap) const;

	const IconTheme m_iconTheme;
};