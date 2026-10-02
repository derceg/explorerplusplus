// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class ResourceLoader;

// These values are used when loading and saving data and shouldn't be changed.
enum class ViewMode
{
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
};

bool IsThumbnailsViewMode(ViewMode viewMode);

UINT GetViewModeMenuId(ViewMode viewMode);
std::wstring GetViewModeMenuText(const ResourceLoader *resourceLoader, ViewMode viewMode);
UINT GetViewModeMenuStringId(ViewMode viewMode);
