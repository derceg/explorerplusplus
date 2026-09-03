// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/ImageOperations.h"
#include "ImageTestHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class ImageOperationsTest : public Test
{
protected:
	enum class ResizeType
	{
		Resize,
		ResizeToFit
	};

	struct ImageSize
	{
		UINT width;
		UINT height;
	};

	void VerifyResize(ResizeType resizeType, const ImageSize &originalSize,
		const ImageSize &targetSize, const ImageSize &expectedSize)
	{
		auto bitmap = BuildTestWICBitmap(originalSize.width, originalSize.height);

		wil::com_ptr_nothrow<IWICBitmapSource> resizedBitmap;

		if (resizeType == ResizeType::Resize)
		{
			ASSERT_HRESULT_SUCCEEDED(ImageOperations::ResizeBitmap(bitmap.get(), targetSize.width,
				targetSize.height, WICBitmapInterpolationModeNearestNeighbor, resizedBitmap));
		}
		else
		{
			ASSERT_HRESULT_SUCCEEDED(
				ImageOperations::ResizeBitmapToFit(bitmap.get(), targetSize.width,
					targetSize.height, WICBitmapInterpolationModeNearestNeighbor, resizedBitmap));
		}

		UINT outputWidth;
		UINT outputHeight;
		ASSERT_HRESULT_SUCCEEDED(resizedBitmap->GetSize(&outputWidth, &outputHeight));
		EXPECT_EQ(outputWidth, expectedSize.width);
		EXPECT_EQ(outputHeight, expectedSize.height);
	}
};

TEST_F(ImageOperationsTest, ResizeBitmapToFit)
{
	VerifyResize(ResizeType::ResizeToFit, { 100, 50 }, { 50, 50 }, { 50, 25 });
	VerifyResize(ResizeType::ResizeToFit, { 50, 100 }, { 50, 50 }, { 25, 50 });
	VerifyResize(ResizeType::ResizeToFit, { 25, 25 }, { 50, 50 }, { 25, 25 });
}

TEST_F(ImageOperationsTest, ResizeBitmap)
{
	ImageSize updatedSize = { 140, 390 };
	VerifyResize(ResizeType::Resize, { 10, 10 }, updatedSize, updatedSize);
}
