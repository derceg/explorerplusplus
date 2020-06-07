// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "../Explorer++/ViewModeHelper.h"
#include <gtest/gtest.h>
#include <array>

// clang-format off
std::array<ViewMode, 8> viewModes = {
	ViewMode::ExtraLargeIcons,
	ViewMode::LargeIcons,
	ViewMode::Icons,
	ViewMode::SmallIcons,
	ViewMode::List,
	ViewMode::Details,
	ViewMode::Thumbnails,
	ViewMode::Tiles
};
// clang-format on

TEST(ViewModeHelperTest, TestGetNext)
{
	ViewMode viewMode = GetNextViewMode(viewModes, ViewMode::List);
	EXPECT_TRUE(viewMode == +ViewMode::SmallIcons);

	viewMode = GetNextViewMode(viewModes, ViewMode::ExtraLargeIcons);
	EXPECT_TRUE(viewMode == +ViewMode::Tiles);
}

TEST(ViewModeHelperTest, TestGetPrevious)
{
	ViewMode viewMode = GetPreviousViewMode(viewModes, ViewMode::LargeIcons);
	EXPECT_TRUE(viewMode == +ViewMode::Icons);

	viewMode = GetPreviousViewMode(viewModes, ViewMode::Tiles);
	EXPECT_TRUE(viewMode == +ViewMode::ExtraLargeIcons);
}