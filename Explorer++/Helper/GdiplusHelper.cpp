// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "GdiplusHelper.h"
#include "ScopedBitmapLock.h"
#include <gdiplus.h>

namespace GdiplusHelper
{

std::optional<CLSID> GetEncoderClsid(const std::wstring &format)
{
	UINT numEncoders;
	UINT size;
	auto res = Gdiplus::GetImageEncodersSize(&numEncoders, &size);

	if (res != Gdiplus::Ok)
	{
		return std::nullopt;
	}

	std::vector<std::byte> rawData(size);
	auto *codecs = reinterpret_cast<Gdiplus::ImageCodecInfo *>(rawData.data());

	res = Gdiplus::GetImageEncoders(numEncoders, size, codecs);

	if (res != Gdiplus::Ok)
	{
		return std::nullopt;
	}

	for (UINT i = 0; i < numEncoders; i++)
	{
		if (codecs[i].MimeType == format)
		{
			return codecs[i].Clsid;
		}
	}

	return std::nullopt;
}

// The Gdiplus::Bitmap class has a number of constructors that accept existing pixel data. The
// resulting object will then silently refer to that data, without ever actually copying it. That's
// generally problematic, since the Gdiplus::Bitmap instance is effectively taking ownership of the
// data in an invisible and undocumented way.
// The Gdiplus::Bitmap class has a Clone() method that creates a copy of a bitmap. Unfortunately,
// that method performs a shallow copy if it can (i.e. if the clone is of the whole image in the
// original pixel format). That's also undocumented and again creates an invisible dependency on the
// data used to create the original bitmap.
// This function exists purely to create a deep copy of a bitmap.
std::unique_ptr<Gdiplus::Bitmap> DeepCopyBitmap(Gdiplus::Bitmap *bitmap)
{
	auto copiedBitmap = std::make_unique<Gdiplus::Bitmap>(bitmap->GetWidth(), bitmap->GetHeight(),
		bitmap->GetPixelFormat());

	if (copiedBitmap->GetLastStatus() != Gdiplus::Ok)
	{
		return nullptr;
	}

	Gdiplus::Rect rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
	ScopedBitmapLock bitmapLock(bitmap, &rect, ScopedBitmapLock::LockMode::Read,
		bitmap->GetPixelFormat());
	ScopedBitmapLock copiedBitmapLock(copiedBitmap.get(), &rect, ScopedBitmapLock::LockMode::Write,
		bitmap->GetPixelFormat());

	auto *bitmapData = bitmapLock.GetBitmapData();
	auto *copiedBitmapData = copiedBitmapLock.GetBitmapData();

	if (!bitmapData || !copiedBitmapData)
	{
		return nullptr;
	}

	// The bitmaps are in the same pixel format, so the stride should be the same (that's also an
	// implicit requirement of the loop below).
	CHECK_EQ(bitmapData->Stride, copiedBitmapData->Stride);

	auto bitmapScanLine = static_cast<std::byte *>(bitmapData->Scan0);
	auto copiedBitmapScanLine = static_cast<std::byte *>(copiedBitmapData->Scan0);

	for (UINT y = 0; y < bitmapData->Height; y++)
	{
		std::memcpy(copiedBitmapScanLine, bitmapScanLine, std::abs(bitmapData->Stride));

		bitmapScanLine += bitmapData->Stride;
		copiedBitmapScanLine += copiedBitmapData->Stride;
	}

	return copiedBitmap;
}

}
