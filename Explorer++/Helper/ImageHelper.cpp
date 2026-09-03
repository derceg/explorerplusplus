// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ImageHelper.h"
#include "ImageInterop.h"
#include "ResourceHelper.h"
#include <wil/com.h>
#include <Shlwapi.h>

namespace ImageHelper
{

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

HRESULT CreateHBITMAPFromImageListIcon(IImageList *imageList, int iconIndex,
	wil::unique_hbitmap &outputBitmap)
{
	wil::unique_hicon icon;
	RETURN_IF_FAILED(imageList->GetIcon(iconIndex, ILD_NORMAL, &icon));

	wil::com_ptr_nothrow<IWICBitmapSource> wicIcon;
	RETURN_IF_FAILED(ImageInterop::CreateWICBitmapFromHICON(icon.get(), wicIcon));

	RETURN_IF_FAILED(ImageInterop::CreateHBITMAPFromWICBitmap(wicIcon.get(), outputBitmap));

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

wil::unique_hbitmap GdiplusBitmapToBitmap(Gdiplus::Bitmap *gdiplusBitmap)
{
	wil::unique_hbitmap bitmap;
	Gdiplus::Status status = gdiplusBitmap->GetHBITMAP(Gdiplus::Color(), &bitmap);

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
