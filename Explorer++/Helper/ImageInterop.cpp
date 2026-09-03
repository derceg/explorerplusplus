// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ImageInterop.h"
#include "GraphicsFactories.h"
#include "Helper.h"

namespace ImageInterop
{

HRESULT CreateWICBitmapFromHICON(HICON source, wil::com_ptr_nothrow<IWICBitmapSource> &output)
{
	wil::com_ptr_nothrow<IWICBitmap> bitmap;
	auto *imagingFactory = GraphicsFactories::GetWICFactory();
	RETURN_IF_FAILED(imagingFactory->CreateBitmapFromHICON(source, &bitmap));
	RETURN_IF_FAILED(WICConvertBitmapSource(GUID_WICPixelFormat32bppPBGRA, bitmap.get(), &output));
	return S_OK;
}

HRESULT CreateHBITMAPFromWICBitmap(IWICBitmapSource *source, wil::unique_hbitmap &output)
{
	UINT width;
	UINT height;
	RETURN_IF_FAILED(source->GetSize(&width, &height));

	wil::com_ptr_nothrow<IWICBitmapSource> convertedSource;
	RETURN_IF_FAILED(
		WICConvertBitmapSource(GUID_WICPixelFormat32bppPBGRA, source, &convertedSource));

	WORD bpp = 32;

	BITMAPINFO bitmapInfo = {};
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfo.bmiHeader.biWidth = width;
	bitmapInfo.bmiHeader.biHeight = -CheckedNumericCast<LONG>(height); // Create a top-down DIB.
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = bpp;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;

	void *bitmapBits = nullptr;
	wil::unique_hbitmap bitmap(
		CreateDIBSection(nullptr, &bitmapInfo, DIB_RGB_COLORS, &bitmapBits, nullptr, 0));

	if (!bitmap)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// See
	// https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader#calculating-surface-stride.
	UINT stride = ((((width * bpp) + 31) & ~31) >> 3);

	RETURN_IF_FAILED(convertedSource->CopyPixels(nullptr, stride, stride * height,
		static_cast<BYTE *>(bitmapBits)));

	output = std::move(bitmap);

	return S_OK;
}

}
