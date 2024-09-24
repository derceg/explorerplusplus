// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ImageTestHelper.h"
#include "../Helper/ImageHelper.h"
#include "../Helper/ScopedBitmapLock.h"
#include <gtest/gtest.h>

void BuildTestBitmap(int width, int height, wil::unique_hbitmap &bitmap)
{
	std::unique_ptr<Gdiplus::Bitmap> gdiplusBitmap;
	BuildTestGdiplusBitmap(width, height, gdiplusBitmap);

	bitmap = ImageHelper::GdiplusBitmapToBitmap(gdiplusBitmap.get());
	ASSERT_NE(bitmap, nullptr);
}

void BuildTestGdiplusBitmap(int width, int height, std::unique_ptr<Gdiplus::Bitmap> &bitmap)
{
	bitmap = std::make_unique<Gdiplus::Bitmap>(width, height, PixelFormat32bppARGB);
	ASSERT_EQ(bitmap->GetLastStatus(), Gdiplus::Ok);

	Gdiplus::SolidBrush brush(static_cast<Gdiplus::ARGB>(Gdiplus::Color::White));

	Gdiplus::Graphics graphics(bitmap.get());
	ASSERT_EQ(bitmap->GetLastStatus(), Gdiplus::Ok);

	auto status = graphics.FillRectangle(&brush, 0, 0, width, height);
	ASSERT_EQ(status, Gdiplus::Ok);
}

bool AreGdiplusBitmapsEquivalent(Gdiplus::Bitmap *bitmap1, Gdiplus::Bitmap *bitmap2)
{
	if ((bitmap1->GetWidth() != bitmap2->GetWidth())
		|| (bitmap1->GetHeight() != bitmap2->GetHeight()))
	{
		return false;
	}

	Gdiplus::Rect rect(0, 0, bitmap1->GetWidth(), bitmap1->GetHeight());
	ScopedBitmapLock bitmapLock1(bitmap1, &rect, ScopedBitmapLock::LockMode::Read,
		PixelFormat32bppARGB);
	ScopedBitmapLock bitmapLock2(bitmap2, &rect, ScopedBitmapLock::LockMode::Read,
		PixelFormat32bppARGB);

	auto *bitmapData1 = bitmapLock1.GetBitmapData();
	auto *bitmapData2 = bitmapLock2.GetBitmapData();

	if (!bitmapData1 || !bitmapData2)
	{
		return false;
	}

	auto *scanLine1 = static_cast<std::byte *>(bitmapData1->Scan0);
	auto *scanLine2 = static_cast<std::byte *>(bitmapData2->Scan0);

	for (UINT y = 0; y < bitmapData1->Height; y++)
	{
		// PixelFormat32bppARGB is used, so there are 4 bytes per pixel.
		int res = std::memcmp(scanLine1, scanLine2, bitmapData1->Width * 4);

		if (res != 0)
		{
			return false;
		}

		scanLine1 += bitmapData1->Stride;
		scanLine2 += bitmapData2->Stride;
	}

	return true;
}
