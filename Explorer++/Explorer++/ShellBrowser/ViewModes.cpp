// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ViewModes.h"
#include "MainResource.h"

int GetViewModeMenuId(ViewMode viewMode)
{
	switch (viewMode)
	{
	case ViewMode::VM_THUMBNAILS:
		return IDM_VIEW_THUMBNAILS;
		break;

	case ViewMode::VM_TILES:
		return IDM_VIEW_TILES;
		break;

	case ViewMode::VM_EXTRALARGEICONS:
		return IDM_VIEW_EXTRALARGEICONS;
		break;

	case ViewMode::VM_LARGEICONS:
		return IDM_VIEW_LARGEICONS;
		break;

	case ViewMode::VM_ICONS:
		return IDM_VIEW_ICONS;
		break;

	case ViewMode::VM_SMALLICONS:
		return IDM_VIEW_SMALLICONS;
		break;

	case ViewMode::VM_LIST:
		return IDM_VIEW_LIST;
		break;

	case ViewMode::VM_DETAILS:
		return IDM_VIEW_DETAILS;
		break;
	}

	return -1;
}

int GetViewModeMenuStringId(ViewMode viewMode)
{
	switch (viewMode)
	{
	case ViewMode::VM_THUMBNAILS:
		return IDS_VIEW_THUMBNAILS;
		break;

	case ViewMode::VM_TILES:
		return IDS_VIEW_TILES;
		break;

	case ViewMode::VM_EXTRALARGEICONS:
		return IDS_VIEW_EXTRALARGEICONS;
		break;

	case ViewMode::VM_LARGEICONS:
		return IDS_VIEW_LARGEICONS;
		break;

	case ViewMode::VM_ICONS:
		return IDS_VIEW_MEDIUMICONS;
		break;

	case ViewMode::VM_SMALLICONS:
		return IDS_VIEW_SMALLICONS;
		break;

	case ViewMode::VM_LIST:
		return IDS_VIEW_LIST;
		break;

	case ViewMode::VM_DETAILS:
		return IDS_VIEW_DETAILS;
		break;
	}

	return -1;
}