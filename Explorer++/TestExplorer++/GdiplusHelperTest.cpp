// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/GdiplusHelper.h"
#include <gtest/gtest.h>
#include <gdiplus.h>

class GdiplusTest : public testing::Test
{
protected:
	void SetUp() override
	{
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		Gdiplus::Status status =
			Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr);
		ASSERT_EQ(status, Gdiplus::Ok);
	}

	void TearDown() override
	{
		Gdiplus::GdiplusShutdown(m_gdiplusToken);
	}

	ULONG_PTR m_gdiplusToken;
};

TEST_F(GdiplusTest, GetEncoderClsid)
{
	// As per
	// https://learn.microsoft.com/en-au/windows/win32/gdiplus/-gdiplus-using-image-encoders-and-decoders-use,
	// the PNG and JPEG encoders are built-in, so they should be safe to assume that they're
	// available here and that GetEncoderClsid() should succeed.
	auto pngClsid = GdiplusHelper::GetEncoderClsid(L"image/png");
	EXPECT_TRUE(pngClsid.has_value());

	auto jpegClsid = GdiplusHelper::GetEncoderClsid(L"image/jpeg");
	EXPECT_TRUE(jpegClsid.has_value());
}
