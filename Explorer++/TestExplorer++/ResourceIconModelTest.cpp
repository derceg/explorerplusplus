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
		m_iconModel(Icon::Refresh, DEFAULT_SIZE, &m_resourceLoader)
	{
	}

	void CheckRetrievedBitmapDimensionsForScale(int scale)
	{
		auto bitmap = m_iconModel.GetBitmap(USER_DEFAULT_SCREEN_DPI * scale, {});
		ASSERT_NE(bitmap, nullptr);

		BITMAP bitmapInfo;
		int res = GetObject(bitmap.get(), sizeof(bitmapInfo), &bitmapInfo);
		ASSERT_NE(res, 0);

		EXPECT_EQ(bitmapInfo.bmWidth, DEFAULT_SIZE * scale);
		EXPECT_EQ(bitmapInfo.bmHeight, DEFAULT_SIZE * scale);
	}

private:
	static constexpr int DEFAULT_SIZE = 16;

	Win32ResourceLoader m_resourceLoader;
	ResourceIconModel m_iconModel;
};

TEST_F(ResourceIconModelTest, GetBitmap)
{
	CheckRetrievedBitmapDimensionsForScale(1);
	CheckRetrievedBitmapDimensionsForScale(2);
	CheckRetrievedBitmapDimensionsForScale(3);
}
