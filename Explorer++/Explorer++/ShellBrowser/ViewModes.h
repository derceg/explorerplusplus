// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BetterEnumsWrapper.h"

class ResourceLoader;

BETTER_ENUM(ViewMode, int,
	Icons = 1,
	SmallIcons = 2,
	List = 3,
	Details = 4,
	Tiles = 5,
	Thumbnails = 6,
	ExtraLargeIcons = 7,
	LargeIcons = 8,
	ExtraLargeThumbnails = 9,
	LargeThumbnails = 10
)

bool IsThumbnailsViewMode(ViewMode viewMode);

UINT GetViewModeMenuId(ViewMode viewMode);
std::wstring GetViewModeMenuText(const ResourceLoader *resourceLoader, ViewMode viewMode);
UINT GetViewModeMenuStringId(ViewMode viewMode);
