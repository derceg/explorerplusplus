// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/ImageHelper.h"
#include "TestResources.h"
#include "../Helper/GraphicsFactories.h"
#include <gtest/gtest.h>

TEST(ImageHelperTest, LoadBitmapFromPNGResource)
{
	wil::com_ptr_nothrow<IWICBitmapSource> bitmap;
	ASSERT_HRESULT_SUCCEEDED(
		ImageHelper::LoadBitmapFromPNGResource(GetModuleHandle(nullptr), IDB_BASIC_PNG, bitmap));

	// This is a basic sanity check that the png has been loaded correctly.
	UINT width;
	UINT height;
	ASSERT_HRESULT_SUCCEEDED(bitmap->GetSize(&width, &height));
	EXPECT_EQ(width, 16U);
	EXPECT_EQ(height, 16U);
}

TEST(ImageHelperTest, InvertBitmap)
{
	// clang-format off
	std::array<BYTE, 16> sourcePixels = {
		0, 64, 255, 255,
		10, 100, 200, 128,
		255, 128, 0, 0,
		40, 50, 60, 64
	};
	// clang-format on

	auto *factory = GraphicsFactories::GetWICFactory();

	wil::com_ptr_nothrow<IWICBitmap> source;
	ASSERT_HRESULT_SUCCEEDED(factory->CreateBitmapFromMemory(2, 2, GUID_WICPixelFormat32bppBGRA, 8,
		static_cast<UINT>(sourcePixels.size()), sourcePixels.data(), &source));

	wil::com_ptr_nothrow<IWICBitmapSource> invertedOutput;
	ASSERT_HRESULT_SUCCEEDED(ImageHelper::InvertBitmap(source.get(), invertedOutput));

	UINT width;
	UINT height;
	ASSERT_HRESULT_SUCCEEDED(invertedOutput->GetSize(&width, &height));
	EXPECT_EQ(width, 2u);
	EXPECT_EQ(height, 2u);

	wil::com_ptr_nothrow<IWICBitmapSource> convertedOutput;
	ASSERT_HRESULT_SUCCEEDED(WICConvertBitmapSource(GUID_WICPixelFormat32bppBGRA,
		invertedOutput.get(), &convertedOutput));

	std::array<BYTE, 16> invertedPixels = {};
	ASSERT_HRESULT_SUCCEEDED(convertedOutput->CopyPixels(nullptr, 8,
		static_cast<UINT>(invertedPixels.size()), invertedPixels.data()));

	// clang-format off
	std::array<BYTE, 16> expectedPixels = {
		255, 191, 0, 255,
		245, 155, 55, 128,
		0, 127, 255, 0,
		215, 205, 195, 64
	};
	// clang-format on

	EXPECT_EQ(invertedPixels, expectedPixels);
}
