// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ResourceIconModel.h"
#include "Win32ResourceLoader.h"
#include <gtest/gtest.h>

using namespace testing;

class ResourceIconModelTest : public Test
{
protected:
	ResourceIconModelTest() :
		m_resourceLoader(GetModuleHandle(nullptr), IconSet::Color, nullptr, nullptr),
		m_iconModel(Icon::Refresh, IconSize::Small, &m_resourceLoader)
	{
	}

	void CheckRetrievedBitmapDimensionsForScale(int scale)
	{
		auto bitmap = m_iconModel.GetBitmap(USER_DEFAULT_SCREEN_DPI * scale, {});
		ASSERT_NE(bitmap, nullptr);

		UINT width;
		UINT height;
		ASSERT_HRESULT_SUCCEEDED(bitmap->GetSize(&width, &height));

		int defaultPixelSize = GetIconPixelSizeAtDefaultDpi(IconSize::Small);
		UINT expectedPixelSize = static_cast<UINT>(defaultPixelSize * scale);
		EXPECT_EQ(width, expectedPixelSize);
		EXPECT_EQ(height, expectedPixelSize);
	}

private:
	Win32ResourceLoader m_resourceLoader;
	ResourceIconModel m_iconModel;
};

TEST_F(ResourceIconModelTest, GetBitmap)
{
	CheckRetrievedBitmapDimensionsForScale(1);
	CheckRetrievedBitmapDimensionsForScale(2);
	CheckRetrievedBitmapDimensionsForScale(3);
}
