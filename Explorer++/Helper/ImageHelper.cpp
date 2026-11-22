// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ImageHelper.h"
#include "GraphicsFactories.h"
#include "ImageInterop.h"
#include "ResourceHelper.h"
#include <wil/com.h>
#include <Shlwapi.h>

namespace ImageHelper
{

HRESULT LoadBitmapFromPNGResource(HINSTANCE resourceInstance, UINT resourceId,
	wil::com_ptr_nothrow<IWICBitmapSource> &output)
{
	auto resourceData = CopyResource(resourceInstance, resourceId, L"PNG");

	if (!resourceData)
	{
		return E_FAIL;
	}

	wil::com_ptr_nothrow<IStream> stream(
		SHCreateMemStream(reinterpret_cast<const BYTE *>(resourceData->data()),
			static_cast<UINT>(resourceData->size())));
	RETURN_IF_NULL_ALLOC(stream);

	RETURN_IF_FAILED(LoadBitmapFromStream(stream.get(), output));

	return S_OK;
}

HRESULT LoadBitmapFromStream(IStream *stream, wil::com_ptr_nothrow<IWICBitmapSource> &output)
{
	auto *imagingFactory = GraphicsFactories::GetWICFactory();

	wil::com_ptr_nothrow<IWICBitmapDecoder> decoder;
	RETURN_IF_FAILED(imagingFactory->CreateDecoderFromStream(stream, nullptr,
		WICDecodeMetadataCacheOnLoad, &decoder));

	wil::com_ptr_nothrow<IWICBitmapFrameDecode> frame;
	RETURN_IF_FAILED(decoder->GetFrame(0, &frame));

	wil::com_ptr_nothrow<IWICBitmap> bitmap;
	RETURN_IF_FAILED(
		imagingFactory->CreateBitmapFromSource(frame.get(), WICBitmapCacheOnLoad, &bitmap));

	output = bitmap;

	return S_OK;
}

HRESULT InvertBitmap(IWICBitmapSource *source, wil::com_ptr_nothrow<IWICBitmapSource> &output)
{
	auto *imagingFactory = GraphicsFactories::GetWICFactory();

	wil::com_ptr_nothrow<IWICBitmapSource> converted;
	RETURN_IF_FAILED(WICConvertBitmapSource(GUID_WICPixelFormat32bppBGRA, source, &converted));

	wil::com_ptr_nothrow<IWICBitmap> bitmap;
	RETURN_IF_FAILED(
		imagingFactory->CreateBitmapFromSource(converted.get(), WICBitmapCacheOnLoad, &bitmap));

	UINT width;
	UINT height;
	RETURN_IF_FAILED(bitmap->GetSize(&width, &height));

	wil::com_ptr_nothrow<IWICBitmapLock> lock;
	RETURN_IF_FAILED(bitmap->Lock(nullptr, WICBitmapLockRead | WICBitmapLockWrite, &lock));

	UINT stride;
	RETURN_IF_FAILED(lock->GetStride(&stride));

	UINT bufferSize;
	BYTE *pixels;
	RETURN_IF_FAILED(lock->GetDataPointer(&bufferSize, &pixels));

	for (UINT y = 0; y < height; y++)
	{
		auto *pixel = pixels + static_cast<size_t>(y) * stride;

		for (UINT x = 0; x < width; x++)
		{
			pixel[0] = static_cast<BYTE>(255 - pixel[0]);
			pixel[1] = static_cast<BYTE>(255 - pixel[1]);
			pixel[2] = static_cast<BYTE>(255 - pixel[2]);

			pixel += 4;
		}
	}

	output = bitmap;

	return S_OK;
}

int AddBitmapToImageList(HIMAGELIST imageList, IWICBitmapSource *bitmap)
{
	// Image lists require non-premultiplied alpha.
	wil::unique_hbitmap gdiBitmap;
	HRESULT hr = ImageInterop::CreateHBITMAPFromWICBitmap(bitmap, ImageInterop::AlphaMode::Straight,
		gdiBitmap);
	CHECK(SUCCEEDED(hr));

	int imageIndex = ImageList_Add(imageList, gdiBitmap.get(), nullptr);
	CHECK_NE(imageIndex, -1);

	return imageIndex;
}

HRESULT CreateBitmapFromImageListIcon(IImageList *imageList, int iconIndex,
	wil::com_ptr_nothrow<IWICBitmapSource> &output)
{
	wil::unique_hicon icon;
	RETURN_IF_FAILED(imageList->GetIcon(iconIndex, ILD_NORMAL, &icon));
	RETURN_IF_FAILED(ImageInterop::CreateWICBitmapFromHICON(icon.get(), output));
	return S_OK;
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

}
