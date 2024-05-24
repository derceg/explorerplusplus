// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/DataExchangeHelper.h"
#include "GdiplusTestHelper.h"
#include <gtest/gtest.h>

TEST(ReadPngDataFromGlobal, UseBitmapAfterGlobalFree)
{
	std::unique_ptr<Gdiplus::Bitmap> bitmap;
	BuildTestBitmap(100, 100, bitmap);

	auto global = WritePngDataToGlobal(bitmap.get());
	ASSERT_NE(global, nullptr);

	auto retrievedBitmap = ReadPngDataFromGlobal(global.get());
	ASSERT_NE(retrievedBitmap, nullptr);

	// One way to implement ReadPngDataFromGlobal() is by calling CreateStreamOnHGlobal() on the
	// HGLOBAL parameter that's provided. However, that method is potentially dangerous, as the
	// stream then implicitly relies on the HGLOBAL. Freeing the HGLOBAL while the stream is still
	// active (i.e. while the Gdiplus::Bitmap still exists) is invalid.
	// The lines below test that freeing the HGLOBAL, then cloning the Gdiplus::Bitmap in a
	// different pixel format (which requires reading the underlying stream) doesn't result in a
	// UAF.
	global.reset();

	Gdiplus::Rect rect(0, 0, retrievedBitmap->GetWidth(), retrievedBitmap->GetHeight());
	Gdiplus::PixelFormat clonedPixelFormat = PixelFormat24bppRGB;
	ASSERT_NE(retrievedBitmap->GetPixelFormat(), clonedPixelFormat);

	std::unique_ptr<Gdiplus::Bitmap> clonedBitmap(retrievedBitmap->Clone(rect, clonedPixelFormat));
	ASSERT_NE(clonedBitmap, nullptr);
}
