// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/ImageHelper.h"
#include "GdiplusTestHelper.h"
#include "TestResources.h"
#include <gtest/gtest.h>

TEST(ImageHelper, LoadGdiplusBitmapFromPNG)
{
	auto png = ImageHelper::LoadGdiplusBitmapFromPNG(GetModuleHandle(nullptr), IDB_BASIC_PNG);
	ASSERT_NE(png, nullptr);

	// This is a basic sanity check that the png has been loaded correctly.
	EXPECT_EQ(png->GetWidth(), 16U);
	EXPECT_EQ(png->GetHeight(), 16U);
}

TEST(ImageHelper, ConvertIconToBitmap)
{
	std::unique_ptr<Gdiplus::Bitmap> gdiplusBitmap;
	BuildTestBitmap(100, 100, gdiplusBitmap);

	wil::unique_hicon icon;
	Gdiplus::Status status = gdiplusBitmap->GetHICON(&icon);
	ASSERT_EQ(status, Gdiplus::Status::Ok);

	wil::unique_hbitmap convertedBitmap;
	EXPECT_HRESULT_SUCCEEDED(ImageHelper::ConvertIconToBitmap(icon.get(),
		GUID_WICPixelFormat32bppPBGRA, convertedBitmap));
	EXPECT_NE(convertedBitmap, nullptr);
}
