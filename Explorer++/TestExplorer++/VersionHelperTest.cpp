// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "VersionHelper.h"
#include "Version.h"
#include "VersionConstants.h"
#include <gtest/gtest.h>

TEST(VersionHelperTest, GetVersion)
{
	const auto &version = VersionHelper::GetVersion();
	const auto &segments = version.GetSegments();
	ASSERT_EQ(segments.size(), 4u);
	EXPECT_EQ(segments[0], static_cast<uint32_t>(MAJOR_VERSION));
	EXPECT_EQ(segments[1], static_cast<uint32_t>(MINOR_VERSION));
	EXPECT_EQ(segments[2], static_cast<uint32_t>(MICRO_VERSION));
	EXPECT_EQ(segments[3], static_cast<uint32_t>(BUILD_VERSION));
}
