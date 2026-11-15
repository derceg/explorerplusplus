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

HRESULT CreateHICONFromWICBitmap(IWICBitmapSource *source, wil::unique_hicon &output)
{
	// It doesn't appear to be explicitly documented, but it appears that CreateIconIndirect() will
	// apply premultiplication itself. See https://github.com/wxWidgets/wxWidgets/issues/9050. That
	// means that if the bitmap passed to that function is premultiplied, then premultiplication
	// will be applied twice, resulting in partially transparent areas being darker than they
	// otherwise would be.
	//
	// Therefore, the conversion to HBITMAP here is done without premultiplication.
	wil::unique_hbitmap gdiBitmap;
	RETURN_IF_FAILED(CreateHBITMAPFromWICBitmap(source, AlphaMode::Straight, gdiBitmap));

	UINT width;
	UINT height;
	RETURN_IF_FAILED(source->GetSize(&width, &height));

	// The bitmap will always be in 32-bit form, with an alpha channel, so the mask will be ignored
	// and therefore doesn't require any pixel data to be set. Note that if the image were fully
	// transparent (i.e. all alpha values were zero), the mask would be required. But it can be
	// safely assumed that's never going to happen here.
	wil::unique_hbitmap maskBitmap(CreateBitmap(width, height, 1, 1, nullptr));
	RETURN_LAST_ERROR_IF_NULL(maskBitmap);

	ICONINFO iconInfo = {};
	iconInfo.fIcon = true;
	iconInfo.hbmColor = gdiBitmap.get();
	iconInfo.hbmMask = maskBitmap.get();
	wil::unique_hicon icon(CreateIconIndirect(&iconInfo));
	RETURN_LAST_ERROR_IF_NULL(icon);

	output = std::move(icon);

	return S_OK;
}

HRESULT CreateHBITMAPFromWICBitmap(IWICBitmapSource *source, AlphaMode alphaMode,
	wil::unique_hbitmap &output)
{
	UINT width;
	UINT height;
	RETURN_IF_FAILED(source->GetSize(&width, &height));

	wil::com_ptr_nothrow<IWICBitmapSource> convertedSource;
	const auto format = (alphaMode == AlphaMode::Premultiplied) ? GUID_WICPixelFormat32bppPBGRA
																: GUID_WICPixelFormat32bppBGRA;
	RETURN_IF_FAILED(WICConvertBitmapSource(format, source, &convertedSource));

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
