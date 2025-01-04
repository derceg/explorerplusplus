// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ImageHelper.h"
#include "Helper.h"
#include "ResourceHelper.h"
#include <wil/com.h>

namespace ImageHelper
{

HRESULT ImageListIconToPBGRABitmap(IImageList *imageList, int iconIndex,
	wil::unique_hbitmap &outputBitmap)
{
	wil::unique_hicon icon;
	RETURN_IF_FAILED(imageList->GetIcon(iconIndex, ILD_NORMAL, &icon));
	RETURN_IF_FAILED(ConvertIconToBitmap(icon.get(), GUID_WICPixelFormat32bppPBGRA, outputBitmap));
	return S_OK;
}

HRESULT ConvertIconToBitmap(HICON icon, WICPixelFormatGUID destPixelFormat,
	wil::unique_hbitmap &outputBitmap)
{
	wil::com_ptr_nothrow<IWICImagingFactory> imagingFactory;
	RETURN_IF_FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&imagingFactory)));

	wil::com_ptr_nothrow<IWICBitmap> wicBitmap;
	RETURN_IF_FAILED(imagingFactory->CreateBitmapFromHICON(icon, &wicBitmap));

	wil::com_ptr_nothrow<IWICBitmapSource> wicConvertedBitmap;
	RETURN_IF_FAILED(WICConvertBitmapSource(destPixelFormat, wicBitmap.get(), &wicConvertedBitmap));

	RETURN_IF_FAILED(
		WicBitmapToBitmap(imagingFactory.get(), wicConvertedBitmap.get(), outputBitmap));

	return S_OK;
}

HRESULT WicBitmapToBitmap(IWICImagingFactory *imagingFactory, IWICBitmapSource *wicBitmapSource,
	wil::unique_hbitmap &outputBitmap)
{
	UINT width;
	UINT height;
	RETURN_IF_FAILED(wicBitmapSource->GetSize(&width, &height));

	WICPixelFormatGUID pixelFormat;
	RETURN_IF_FAILED(wicBitmapSource->GetPixelFormat(&pixelFormat));

	wil::com_ptr_nothrow<IWICComponentInfo> componentInfo;
	RETURN_IF_FAILED(imagingFactory->CreateComponentInfo(pixelFormat, &componentInfo));

	wil::com_ptr_nothrow<IWICPixelFormatInfo> pixelFormatInfo;
	RETURN_IF_FAILED(componentInfo->QueryInterface(IID_PPV_ARGS(&pixelFormatInfo)));

	UINT bpp;
	RETURN_IF_FAILED(pixelFormatInfo->GetBitsPerPixel(&bpp));

	BITMAPINFO bitmapInfo = {};
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfo.bmiHeader.biWidth = width;
	bitmapInfo.bmiHeader.biHeight = -CheckedNumericCast<LONG>(height); // Create a top-down DIB.
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = CheckedNumericCast<WORD>(bpp);
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

	RETURN_IF_FAILED(wicBitmapSource->CopyPixels(nullptr, stride, stride * height,
		static_cast<BYTE *>(bitmapBits)));

	outputBitmap = std::move(bitmap);

	return S_OK;
}

std::unique_ptr<Gdiplus::Bitmap> LoadGdiplusBitmapFromPNG(HINSTANCE resourceInstance,
	UINT resourceId)
{
	auto resourceData = CopyResource(resourceInstance, resourceId, L"PNG");

	if (!resourceData)
	{
		return nullptr;
	}

	wil::com_ptr_nothrow<IStream> stream(
		SHCreateMemStream(reinterpret_cast<const BYTE *>(resourceData->data()),
			static_cast<UINT>(resourceData->size())));

	if (!stream)
	{
		return nullptr;
	}

	auto bitmap = std::make_unique<Gdiplus::Bitmap>(stream.get());

	if (bitmap->GetLastStatus() != Gdiplus::Ok)
	{
		return nullptr;
	}

	return bitmap;
}

int CopyImageListIcon(HIMAGELIST destination, HIMAGELIST source, int sourceIconIndex)
{
	// Note that the return values below are CHECK'd. Although both functions can fail due to
	// allocation failures, the images being copied are small, so it's very unlikely that any
	// allocations will fail. And if they do, the application probably isn't going to run very well
	// anyway.
	// That then means that failures are more likely due to programming errors - for example, a
	// sourceIconIndex that's invalid. Catching issues like that here is better than letting
	// execution continue. Doing that would make any potential issues harder to notice and more
	// likely to be silently ignored.
	wil::unique_hicon icon(ImageList_GetIcon(source, sourceIconIndex, ILD_NORMAL));
	CHECK(icon);

	int index = ImageList_AddIcon(destination, icon.get());
	CHECK_NE(index, -1);

	return index;
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
