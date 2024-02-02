// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ViewModes.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include <glog/logging.h>

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

	default:
		LOG(FATAL) << "ViewMode value not found";
	}
}

std::wstring GetViewModeMenuText(ViewMode viewMode, HINSTANCE resourceInstance)
{
	UINT stringId;

	switch (viewMode)
	{
	case ViewMode::Thumbnails:
		stringId = IDS_VIEW_THUMBNAILS;
		break;

	case ViewMode::Tiles:
		stringId = IDS_VIEW_TILES;
		break;

	case ViewMode::ExtraLargeIcons:
		stringId = IDS_VIEW_EXTRALARGEICONS;
		break;

	case ViewMode::LargeIcons:
		stringId = IDS_VIEW_LARGEICONS;
		break;

	case ViewMode::Icons:
		stringId = IDS_VIEW_MEDIUMICONS;
		break;

	case ViewMode::SmallIcons:
		stringId = IDS_VIEW_SMALLICONS;
		break;

	case ViewMode::List:
		stringId = IDS_VIEW_LIST;
		break;

	case ViewMode::Details:
		stringId = IDS_VIEW_DETAILS;
		break;

	default:
		LOG(FATAL) << "ViewMode value not found";
		__assume(0);
	}

	return ResourceHelper::LoadString(resourceInstance, stringId);
}
