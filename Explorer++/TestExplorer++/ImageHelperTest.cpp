// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/ImageHelper.h"
#include "TestResources.h"
#include <gtest/gtest.h>

TEST(LoadGdiplusBitmapFromPNG, Basic)
{
	auto png = ImageHelper::LoadGdiplusBitmapFromPNG(GetModuleHandle(nullptr), IDB_BASIC_PNG);
	ASSERT_NE(png, nullptr);

	// This is a basic sanity check that the png has been loaded correctly.
	EXPECT_EQ(png->GetWidth(), 16U);
	EXPECT_EQ(png->GetHeight(), 16U);
}
