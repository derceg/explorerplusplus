// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <Uxtheme.h>
#include <gdiplus.h>

typedef DWORD ARGB;

namespace ImageHelper
{

wil::unique_hbitmap ImageListIconToBitmap(IImageList *imageList, int iconIndex);
void InitBitmapInfo(__out_bcount(cbInfo) BITMAPINFO *pbmi, ULONG cbInfo, LONG cx, LONG cy,
	WORD bpp);
HRESULT Create32BitHBITMAP(HDC hdc, const SIZE *psize, __deref_opt_out void **ppvBits,
	__out HBITMAP *phBmp);
HRESULT ConvertToPARGB32(HDC hdc, __inout ARGB *pargb, HBITMAP hbmp, SIZE &sizImage, int cxRow);
bool HasAlpha(__in ARGB *pargb, SIZE &sizImage, int cxRow);
HRESULT ConvertBufferToPARGB32(HPAINTBUFFER hPaintBuffer, HDC hdc, HICON hicon, SIZE &sizIcon);
HBITMAP IconToBitmapPARGB32(HICON hicon, int width, int height);

std::unique_ptr<Gdiplus::Bitmap> LoadGdiplusBitmapFromPNG(HINSTANCE resourceInstance,
	UINT resourceId);
int CopyImageListIcon(HIMAGELIST destination, HIMAGELIST source, int sourceIconIndex);

wil::unique_hbitmap GdiplusBitmapToBitmap(Gdiplus::Bitmap *gdiplusBitmap);
wil::unique_hicon GdiplusBitmapToIcon(Gdiplus::Bitmap *gdiplusBitmap);

}
