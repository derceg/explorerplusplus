// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BetterEnumsWrapper.h"

// clang-format off
BETTER_ENUM(ViewMode, int,
	Icons = 1,
	SmallIcons = 2,
	List = 3,
	Details = 4,
	Tiles = 5,
	Thumbnails = 6,
	ExtraLargeIcons = 7,
	LargeIcons = 8
)
// clang-format on

UINT GetViewModeMenuId(ViewMode viewMode);
std::wstring GetViewModeMenuText(ViewMode viewMode, HINSTANCE resourceInstance);
