// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/D2DHelper.h"
#include <gtest/gtest.h>

using namespace testing;

TEST(D2DHelperTest, DrawToBitmap)
{
	UINT width = 100;
	UINT height = 60;
	wil::com_ptr_nothrow<IWICBitmapSource> bitmap;
	HRESULT hr = D2DHelper::DrawToBitmap(
		width, height, D2DHelper::GdiUsage::WontUse,
		[](ID2D1RenderTarget *renderTarget)
		{
			renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Blue));
			return S_OK;
		},
		bitmap);
	ASSERT_HRESULT_SUCCEEDED(hr);

	UINT outputWidth;
	UINT outputHeight;
	hr = bitmap->GetSize(&outputWidth, &outputHeight);
	ASSERT_HRESULT_SUCCEEDED(hr);
	EXPECT_EQ(outputWidth, width);
	EXPECT_EQ(outputHeight, height);

	std::vector<uint32_t> pixels(width * height);
	hr = bitmap->CopyPixels(nullptr, width * sizeof(uint32_t),
		static_cast<UINT>(pixels.size() * sizeof(uint32_t)),
		reinterpret_cast<BYTE *>(pixels.data()));
	ASSERT_HRESULT_SUCCEEDED(hr);

	// The draw call above should have filled the bitmap with opaque blue.
	EXPECT_THAT(pixels, Each(0xFF0000FF));
}
