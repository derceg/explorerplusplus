// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "LanguageHelper.h"
#include <gtest/gtest.h>

TEST(LanguageHelperTest, NotRTL)
{
	EXPECT_FALSE(LanguageHelper::IsLanguageRTL(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)));
}

TEST(LanguageHelperTest, RTL)
{
	EXPECT_TRUE(LanguageHelper::IsLanguageRTL(MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_UAE)));
}
