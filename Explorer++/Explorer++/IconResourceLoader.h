// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BetterEnumsWrapper.h"
#include "Icon.h"
#include <wil/resource.h>
#include <gdiplus.h>

// Note that the values in this enumeration are used when saving/loading the icon theme and should
// not be changed.
// clang-format off
BETTER_ENUM(IconTheme, int,
	Color = 0,
	Windows10 = 1,
	FluentUi = 2
)
// clang-format on

class IconResourceLoader
{
public:
	IconResourceLoader(IconTheme iconTheme);

	wil::unique_hbitmap LoadBitmapFromPNGForDpi(
		Icon icon, int iconWidth, int iconHeight, int dpi) const;
	wil::unique_hbitmap LoadBitmapFromPNGAndScale(Icon icon, int iconWidth, int iconHeight) const;
	wil::unique_hicon LoadIconFromPNGForDpi(
		Icon icon, int iconWidth, int iconHeight, int dpi) const;
	wil::unique_hicon LoadIconFromPNGAndScale(Icon icon, int iconWidth, int iconHeight) const;

private:
	wil::unique_hbitmap RetrieveBitmapFromGdiplusBitmap(Gdiplus::Bitmap *gdiplusBitmap) const;
	wil::unique_hicon RetrieveIconFromGdiplusBitmap(Gdiplus::Bitmap *gdiplusBitmap) const;

	std::unique_ptr<Gdiplus::Bitmap> LoadGdiplusBitmapFromPNGForDpi(
		Icon icon, int iconWidth, int iconHeight, int dpi) const;
	std::unique_ptr<Gdiplus::Bitmap> LoadGdiplusBitmapFromPNGAndScalePlusInvert(
		Icon icon, int iconWidth, int iconHeight) const;
	std::unique_ptr<Gdiplus::Bitmap> LoadGdiplusBitmapFromPNGAndScale(
		Icon icon, int iconWidth, int iconHeight) const;

	const IconTheme m_iconTheme;
};