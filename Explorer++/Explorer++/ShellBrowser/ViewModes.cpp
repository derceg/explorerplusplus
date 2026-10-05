// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ViewModes.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include <glog/logging.h>

bool IsThumbnailsViewMode(ViewMode viewMode)
{
	return (viewMode == ViewMode::Thumbnails) || (viewMode == ViewMode::LargeThumbnails)
		|| (viewMode == ViewMode::ExtraLargeThumbnails);
}

UINT GetViewModeMenuId(ViewMode viewMode)
{
	switch (viewMode)
	{
	case ViewMode::Thumbnails:
		return IDM_VIEW_THUMBNAILS;

	case ViewMode::Tiles:
		return IDM_VIEW_TILES;

	case ViewMode::ExtraLargeIcons:
		return IDM_VIEW_EXTRALARGEICONS;

	case ViewMode::LargeIcons:
		return IDM_VIEW_LARGEICONS;

	case ViewMode::Icons:
		return IDM_VIEW_ICONS;

	case ViewMode::SmallIcons:
		return IDM_VIEW_SMALLICONS;

	case ViewMode::List:
		return IDM_VIEW_LIST;

	case ViewMode::Details:
		return IDM_VIEW_DETAILS;

	case ViewMode::LargeThumbnails:
		return IDM_VIEW_LARGETHUMBNAILS;

	case ViewMode::ExtraLargeThumbnails:
		return IDM_VIEW_EXTRALARGETHUMBNAILS;

	default:
		LOG(FATAL) << "ViewMode value not found";
	}
}

std::wstring GetViewModeMenuText(const ResourceLoader *resourceLoader, ViewMode viewMode)
{
	return resourceLoader->LoadString(GetViewModeMenuStringId(viewMode));
}

UINT GetViewModeMenuStringId(ViewMode viewMode)
{
	switch (viewMode)
	{
	case ViewMode::Icons:
		return IDS_VIEW_MEDIUMICONS;

	case ViewMode::SmallIcons:
		return IDS_VIEW_SMALLICONS;

	case ViewMode::List:
		return IDS_VIEW_LIST;

	case ViewMode::Details:
		return IDS_VIEW_DETAILS;

	case ViewMode::Tiles:
		return IDS_VIEW_TILES;

	case ViewMode::Thumbnails:
		return IDS_VIEW_THUMBNAILS;

	case ViewMode::ExtraLargeIcons:
		return IDS_VIEW_EXTRALARGEICONS;

	case ViewMode::LargeIcons:
		return IDS_VIEW_LARGEICONS;

	case ViewMode::ExtraLargeThumbnails:
		return IDS_VIEW_EXTRALARGETHUMBNAILS;

	case ViewMode::LargeThumbnails:
		return IDS_VIEW_LARGETHUMBNAILS;
	}

	LOG(FATAL) << "Invalid ViewMode value";
}

UINT GetViewModeMenuHelpStringId(ViewMode viewMode)
{
	switch (viewMode)
	{
	case ViewMode::Icons:
		return IDS_VIEW_ICONS_HELP_TEXT;

	case ViewMode::SmallIcons:
		return IDS_VIEW_SMALL_ICONS_HELP_TEXT;

	case ViewMode::List:
		return IDS_VIEW_LIST_HELP_TEXT;

	case ViewMode::Details:
		return IDS_VIEW_DETAILS_HELP_TEXT;

	case ViewMode::Tiles:
		return IDS_VIEW_TILES_HELP_TEXT;

	case ViewMode::Thumbnails:
		return IDS_VIEW_THUMBNAILS_HELP_TEXT;

	case ViewMode::ExtraLargeIcons:
		return IDS_VIEW_EXTRA_LARGE_ICONS_HELP_TEXT;

	case ViewMode::LargeIcons:
		return IDS_VIEW_LARGE_ICONS_HELP_TEXT;

	case ViewMode::ExtraLargeThumbnails:
		return IDS_VIEW_EXTRA_LARGE_THUMBNAILS_HELP_TEXT;

	case ViewMode::LargeThumbnails:
		return IDS_VIEW_LARGE_THUMBNAILS_HELP_TEXT;
	}

	LOG(FATAL) << "Invalid ViewMode value";
}
