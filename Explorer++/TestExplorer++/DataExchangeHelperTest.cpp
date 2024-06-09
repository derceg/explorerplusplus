// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/DataExchangeHelper.h"
#include "GdiplusTestHelper.h"
#include "../Helper/DataObjectImpl.h"
#include <gtest/gtest.h>

using namespace testing;

class ReadImageDataFromGlobalTest : public Test
{
protected:
	enum class ImageType
	{
		Png,
		DIB
	};

	void PerformTest(ImageType imageType)
	{
		std::unique_ptr<Gdiplus::Bitmap> bitmap;
		BuildTestBitmap(100, 100, bitmap);

		wil::unique_hglobal global;

		switch (imageType)
		{
		case ImageType::Png:
			global = WritePngDataToGlobal(bitmap.get());
			break;

		case ImageType::DIB:
			global = WriteDIBDataToGlobal(bitmap.get());
			break;

		default:
			DCHECK(false);
			break;
		}

		ASSERT_NE(global, nullptr);

		std::unique_ptr<Gdiplus::Bitmap> retrievedBitmap;

		switch (imageType)
		{
		case ImageType::Png:
			retrievedBitmap = ReadPngDataFromGlobal(global.get());
			break;

		case ImageType::DIB:
			retrievedBitmap = ReadDIBDataFromGlobal(global.get());
			break;

		default:
			DCHECK(false);
			break;
		}

		ASSERT_NE(retrievedBitmap, nullptr);

		// The Gdiplus::Bitmap constructors tend not to copy data that's passed in, which can create
		// a silent dependency on any input data. The lines below test that freeing the HGLOBAL,
		// then cloning the Gdiplus::Bitmap in a different pixel format (which requires accessing
		// the original data) doesn't result in a UAF.
		// Note that when a DIB image is being processed, asan doesn't currently detect any errors.
		// That's seemingly because the msvcrt.dll!memcpy calls that occur within Clone() aren't
		// being intercepted. That means that even if ReadDIBDataFromGlobal() is returning a bitmap
		// that relies on the input HGLOBAL, no errors will currently be shown in that case.
		global.reset();

		Gdiplus::Rect rect(0, 0, retrievedBitmap->GetWidth(), retrievedBitmap->GetHeight());
		Gdiplus::PixelFormat clonedPixelFormat = PixelFormat24bppRGB;
		ASSERT_NE(retrievedBitmap->GetPixelFormat(), clonedPixelFormat);

		std::unique_ptr<Gdiplus::Bitmap> clonedBitmap(
			retrievedBitmap->Clone(rect, clonedPixelFormat));
		ASSERT_NE(clonedBitmap, nullptr);
	}
};

TEST_F(ReadImageDataFromGlobalTest, UseBitmapAfterGlobalFree)
{
	PerformTest(ImageType::Png);
	PerformTest(ImageType::DIB);
}

TEST(DataExchangeHelperTest, ReadWriteVirtualFiles)
{
	auto dataObject = winrt::make<DataObjectImpl>();

	std::vector<VirtualFile> virtualFiles = { { L"test-file-1.txt", "Contents of file 1" },
		{ L"test-file-2.txt", "Contents of file 2" },
		{ L"test-file-3.txt", "Contents of file 3" } };
	ASSERT_HRESULT_SUCCEEDED(WriteVirtualFilesToDataObject(dataObject.get(), virtualFiles));

	std::vector<VirtualFile> retrievedVirtualFiles;
	ASSERT_HRESULT_SUCCEEDED(
		ReadVirtualFilesFromDataObject(dataObject.get(), retrievedVirtualFiles));

	EXPECT_EQ(retrievedVirtualFiles, virtualFiles);
}
