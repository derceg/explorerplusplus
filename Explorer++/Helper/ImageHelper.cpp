// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ImageHelper.h"
#include "ResourceHelper.h"
#include <wil/com.h>

namespace ImageHelper
{

wil::unique_hbitmap ImageListIconToBitmap(IImageList *imageList, int iconIndex)
{
	wil::unique_hicon icon;
	HRESULT hr = imageList->GetIcon(iconIndex, ILD_NORMAL, &icon);

	if (FAILED(hr))
	{
		return nullptr;
	}

	int iconWidth;
	int iconHeight;
	hr = imageList->GetIconSize(&iconWidth, &iconHeight);

	if (FAILED(hr))
	{
		return nullptr;
	}

	return wil::unique_hbitmap(IconToBitmapPARGB32(icon.get(), iconWidth, iconHeight));
}

void InitBitmapInfo(__out_bcount(cbInfo) BITMAPINFO *pbmi, ULONG cbInfo, LONG cx, LONG cy, WORD bpp)
{
	ZeroMemory(pbmi, cbInfo);
	pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pbmi->bmiHeader.biPlanes = 1;
	pbmi->bmiHeader.biCompression = BI_RGB;

	pbmi->bmiHeader.biWidth = cx;
	pbmi->bmiHeader.biHeight = cy;
	pbmi->bmiHeader.biBitCount = bpp;
}

HRESULT Create32BitHBITMAP(HDC hdc, const SIZE *psize, __deref_opt_out void **ppvBits,
	__out HBITMAP *phBmp)
{
	*phBmp = nullptr;

	BITMAPINFO bmi;
	InitBitmapInfo(&bmi, sizeof(bmi), psize->cx, psize->cy, 32);

	HDC hdcUsed = hdc ? hdc : GetDC(nullptr);
	if (hdcUsed)
	{
		*phBmp = CreateDIBSection(hdcUsed, &bmi, DIB_RGB_COLORS, ppvBits, nullptr, 0);
		if (hdc != hdcUsed)
		{
			ReleaseDC(nullptr, hdcUsed);
		}
	}
	return (nullptr == *phBmp) ? E_OUTOFMEMORY : S_OK;
}

HRESULT ConvertToPARGB32(HDC hdc, __inout ARGB *pargb, HBITMAP hbmp, SIZE &sizImage, int cxRow)
{
	BITMAPINFO bmi;
	InitBitmapInfo(&bmi, sizeof(bmi), sizImage.cx, sizImage.cy, 32);

	HRESULT hr = E_OUTOFMEMORY;
	HANDLE hHeap = GetProcessHeap();
	void *pvBits = HeapAlloc(hHeap, 0, bmi.bmiHeader.biWidth * 4 * bmi.bmiHeader.biHeight);
	if (pvBits)
	{
		hr = E_UNEXPECTED;
		if (GetDIBits(hdc, hbmp, 0, bmi.bmiHeader.biHeight, pvBits, &bmi, DIB_RGB_COLORS)
			== bmi.bmiHeader.biHeight)
		{
			ULONG cxDelta = cxRow - bmi.bmiHeader.biWidth;
			ARGB *pargbMask = static_cast<ARGB *>(pvBits);

			for (ULONG y = bmi.bmiHeader.biHeight; y; --y)
			{
				for (ULONG x = bmi.bmiHeader.biWidth; x; --x)
				{
					if (*pargbMask++)
					{
						// transparent pixel
						*pargb++ = 0;
					}
					else
					{
						// opaque pixel
						*pargb++ |= 0xFF000000;
					}
				}

				pargb += cxDelta;
			}

			hr = S_OK;
		}

		HeapFree(hHeap, 0, pvBits);
	}

	return hr;
}

bool HasAlpha(__in ARGB *pargb, SIZE &sizImage, int cxRow)
{
	ULONG cxDelta = cxRow - sizImage.cx;
	for (ULONG y = sizImage.cy; y; --y)
	{
		for (ULONG x = sizImage.cx; x; --x)
		{
			if (*pargb++ & 0xFF000000)
			{
				return true;
			}
		}

		pargb += cxDelta;
	}

	return false;
}

HRESULT ConvertBufferToPARGB32(HPAINTBUFFER hPaintBuffer, HDC hdc, HICON hicon, SIZE &sizIcon)
{
	RGBQUAD *prgbQuad;
	int cxRow;
	HRESULT hr = GetBufferedPaintBits(hPaintBuffer, &prgbQuad, &cxRow);
	if (SUCCEEDED(hr))
	{
		ARGB *pargb = reinterpret_cast<ARGB *>(prgbQuad);
		if (!HasAlpha(pargb, sizIcon, cxRow))
		{
			ICONINFO info;
			if (GetIconInfo(hicon, &info))
			{
				if (info.hbmMask)
				{
					hr = ConvertToPARGB32(hdc, pargb, info.hbmMask, sizIcon, cxRow);
				}

				DeleteObject(info.hbmColor);
				DeleteObject(info.hbmMask);
			}
		}
	}

	return hr;
}

HBITMAP IconToBitmapPARGB32(HICON hicon, int width, int height)
{
	HRESULT hr = E_OUTOFMEMORY;
	HBITMAP hbmp = nullptr;

	SIZE sizIcon;
	sizIcon.cx = width;
	sizIcon.cy = height;

	RECT rcIcon;
	SetRect(&rcIcon, 0, 0, sizIcon.cx, sizIcon.cy);

	HDC hdcDest = CreateCompatibleDC(nullptr);
	if (hdcDest)
	{
		hr = Create32BitHBITMAP(hdcDest, &sizIcon, nullptr, &hbmp);
		if (SUCCEEDED(hr))
		{
			hr = E_FAIL;

			auto hbmpOld = (HBITMAP) SelectObject(hdcDest, hbmp);
			if (hbmpOld)
			{
				BLENDFUNCTION bfAlpha = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
				BP_PAINTPARAMS paintParams = { 0 };
				paintParams.cbSize = sizeof(paintParams);
				paintParams.dwFlags = BPPF_ERASE;
				paintParams.pBlendFunction = &bfAlpha;

				HDC hdcBuffer;
				HPAINTBUFFER hPaintBuffer =
					BeginBufferedPaint(hdcDest, &rcIcon, BPBF_DIB, &paintParams, &hdcBuffer);
				if (hPaintBuffer)
				{
					if (DrawIconEx(hdcBuffer, 0, 0, hicon, sizIcon.cx, sizIcon.cy, 0, nullptr,
							DI_NORMAL))
					{
						// If icon did not have an alpha channel we need to convert buffer to PARGB
						hr = ConvertBufferToPARGB32(hPaintBuffer, hdcDest, hicon, sizIcon);
					}

					// This will write the buffer contents to the destination bitmap
					EndBufferedPaint(hPaintBuffer, TRUE);
				}

				SelectObject(hdcDest, hbmpOld);
			}
		}

		DeleteDC(hdcDest);
	}

	if (FAILED(hr))
	{
		DeleteObject(hbmp);
		hbmp = nullptr;
	}

	return hbmp;
}

std::unique_ptr<Gdiplus::Bitmap> LoadGdiplusBitmapFromPNG(HINSTANCE resourceInstance,
	UINT resourceId)
{
	auto resourceData = CopyResource(resourceInstance, resourceId, L"PNG");

	if (!resourceData)
	{
		return nullptr;
	}

	wil::com_ptr_nothrow<IStream> stream(SHCreateMemStream(
		reinterpret_cast<BYTE *>(resourceData->data()), static_cast<UINT>(resourceData->size())));

	if (!stream)
	{
		return nullptr;
	}

	return std::make_unique<Gdiplus::Bitmap>(stream.get());
}

int CopyImageListIcon(HIMAGELIST destination, HIMAGELIST source, int sourceIconIndex)
{
	wil::unique_hicon icon(ImageList_GetIcon(source, sourceIconIndex, ILD_NORMAL));

	if (!icon)
	{
		return -1;
	}

	return ImageList_AddIcon(destination, icon.get());
}

wil::unique_hbitmap GdiplusBitmapToBitmap(Gdiplus::Bitmap *gdiplusBitmap)
{
	wil::unique_hbitmap bitmap;
	Gdiplus::Color color(0, 0, 0);
	Gdiplus::Status status = gdiplusBitmap->GetHBITMAP(color, &bitmap);

	if (status != Gdiplus::Status::Ok)
	{
		return nullptr;
	}

	return bitmap;
}

wil::unique_hicon GdiplusBitmapToIcon(Gdiplus::Bitmap *gdiplusBitmap)
{
	wil::unique_hicon hicon;
	Gdiplus::Status status = gdiplusBitmap->GetHICON(&hicon);

	if (status != Gdiplus::Status::Ok)
	{
		return nullptr;
	}

	return hicon;
}

}
