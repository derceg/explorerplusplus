// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <CommCtrl.h>
#include <commoncontrols.h>
#include <gdiplus.h>
#include <wincodec.h>
#include <memory>

namespace ImageHelper
{

HRESULT ImageListIconToPBGRABitmap(IImageList *imageList, int iconIndex,
	wil::unique_hbitmap &outputBitmap);
HRESULT ConvertIconToBitmap(HICON icon, WICPixelFormatGUID destPixelFormat,
	wil::unique_hbitmap &outputBitmap);
HRESULT WicBitmapToBitmap(IWICImagingFactory *imagingFactory, IWICBitmapSource *wicBitmapSource,
	wil::unique_hbitmap &outputBitmap);

std::unique_ptr<Gdiplus::Bitmap> LoadGdiplusBitmapFromPNG(HINSTANCE resourceInstance,
	UINT resourceId);
int CopyImageListIcon(HIMAGELIST destination, HIMAGELIST source, int sourceIconIndex);

wil::unique_hbitmap GdiplusBitmapToBitmap(Gdiplus::Bitmap *gdiplusBitmap);
wil::unique_hicon GdiplusBitmapToIcon(Gdiplus::Bitmap *gdiplusBitmap);

}
