// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ImageTestHelper.h"
#include "../Helper/D2DHelper.h"

wil::com_ptr_nothrow<IWICBitmapSource> BuildTestBitmap(UINT width, UINT height)
{
	wil::com_ptr_nothrow<IWICBitmapSource> bitmap;
	HRESULT hr = D2DHelper::DrawToBitmap(
		width, height, D2DHelper::GdiUsage::WontUse,
		[](ID2D1RenderTarget *renderTarget)
		{
			renderTarget->Clear(D2D1::ColorF(0.62f, 0.75f, 0.43f, 1.0f));
			return S_OK;
		},
		bitmap);
	CHECK(SUCCEEDED(hr));
	return bitmap;
}

bool AreBitmapsEquivalent(IWICBitmapSource *bitmap1, IWICBitmapSource *bitmap2)
{
	UINT bitmap1Width;
	UINT bitmap1Height;
	CHECK(SUCCEEDED(bitmap1->GetSize(&bitmap1Width, &bitmap1Height)));

	UINT bitmap2Width;
	UINT bitmap2Height;
	CHECK(SUCCEEDED(bitmap2->GetSize(&bitmap2Width, &bitmap2Height)));

	if (bitmap1Width != bitmap2Width || bitmap1Height != bitmap2Height)
	{
		return false;
	}

	wil::com_ptr_nothrow<IWICBitmapSource> bitmap1Bgra;
	CHECK(SUCCEEDED(WICConvertBitmapSource(GUID_WICPixelFormat32bppBGRA, bitmap1, &bitmap1Bgra)));

	wil::com_ptr_nothrow<IWICBitmapSource> bitmap2Bgra;
	CHECK(SUCCEEDED(WICConvertBitmapSource(GUID_WICPixelFormat32bppBGRA, bitmap2, &bitmap2Bgra)));

	UINT stride = bitmap1Width * 4;
	UINT byteCount = stride * bitmap1Height;

	std::vector<BYTE> bitmap1Pixels(byteCount);
	CHECK(SUCCEEDED(bitmap1Bgra->CopyPixels(nullptr, stride, byteCount, bitmap1Pixels.data())));

	std::vector<BYTE> bitmap2Pixels(byteCount);
	CHECK(SUCCEEDED(bitmap2Bgra->CopyPixels(nullptr, stride, byteCount, bitmap2Pixels.data())));

	return bitmap1Pixels == bitmap2Pixels;
}
