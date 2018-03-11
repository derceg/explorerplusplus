#pragma once

#include <Uxtheme.h>

typedef DWORD ARGB;

namespace ImageHelper
{
	void InitBitmapInfo(__out_bcount(cbInfo) BITMAPINFO *pbmi, ULONG cbInfo, LONG cx, LONG cy, WORD bpp);
	HRESULT Create32BitHBITMAP(HDC hdc, const SIZE *psize, __deref_opt_out void **ppvBits, __out HBITMAP* phBmp);
	HRESULT ConvertToPARGB32(HDC hdc, __inout ARGB *pargb, HBITMAP hbmp, SIZE& sizImage, int cxRow);
	bool HasAlpha(__in ARGB *pargb, SIZE& sizImage, int cxRow);
	HRESULT ConvertBufferToPARGB32(HPAINTBUFFER hPaintBuffer, HDC hdc, HICON hicon, SIZE& sizIcon);
	HBITMAP IconToBitmapPARGB32(HICON hicon, int width, int height);
}