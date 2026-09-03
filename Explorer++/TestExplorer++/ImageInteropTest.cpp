// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/ImageInterop.h"
#include "ImageTestHelper.h"
#include "ResourceTestHelper.h"
#include <gtest/gtest.h>

using namespace testing;

TEST(ImageInteropTest, CreateWICBitmapFromHICON)
{
	auto iconPath = GetResourcePath(L"16x16-icon.ico");
	wil::unique_hicon icon(static_cast<HICON>(
		LoadImage(nullptr, iconPath.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE)));
	ASSERT_NE(icon, nullptr);

	wil::com_ptr_nothrow<IWICBitmapSource> bitmap;
	HRESULT hr = ImageInterop::CreateWICBitmapFromHICON(icon.get(), bitmap);
	ASSERT_HRESULT_SUCCEEDED(hr);

	UINT width;
	UINT height;
	hr = bitmap->GetSize(&width, &height);
	ASSERT_HRESULT_SUCCEEDED(hr);
	EXPECT_EQ(width, 16u);
	EXPECT_EQ(height, 16u);
}

TEST(ImageInteropTest, CreateHBITMAPFromWICBitmap)
{
	auto bitmap = BuildTestWICBitmap(120, 60);

	wil::unique_hbitmap outputBitmap;
	HRESULT hr = ImageInterop::CreateHBITMAPFromWICBitmap(bitmap.get(), outputBitmap);
	ASSERT_HRESULT_SUCCEEDED(hr);

	BITMAP bitmapInfo;
	auto res = GetObject(outputBitmap.get(), sizeof(bitmapInfo), &bitmapInfo);
	ASSERT_NE(res, 0);
	EXPECT_EQ(bitmapInfo.bmWidth, 120);
	EXPECT_EQ(bitmapInfo.bmHeight, 60);
}
