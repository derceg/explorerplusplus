/******************************************************************
 *
 * Project: Explorer++
 * File: ViewModes.cpp
 * License: GPL - See LICENSE in the top level directory
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "ViewModes.h"
#include "MainResource.h"

int GetViewModeMenuId(UINT uViewMode)
{
	switch (uViewMode)
	{
	case VM_THUMBNAILS:
		return IDM_VIEW_THUMBNAILS;
		break;

	case VM_TILES:
		return IDM_VIEW_TILES;
		break;

	case VM_EXTRALARGEICONS:
		return IDM_VIEW_EXTRALARGEICONS;
		break;

	case VM_LARGEICONS:
		return IDM_VIEW_LARGEICONS;
		break;

	case VM_ICONS:
		return IDM_VIEW_ICONS;
		break;

	case VM_SMALLICONS:
		return IDM_VIEW_SMALLICONS;
		break;

	case VM_LIST:
		return IDM_VIEW_LIST;
		break;

	case VM_DETAILS:
		return IDM_VIEW_DETAILS;
		break;
	}

	return -1;
}

int GetViewModeMenuStringId(UINT uViewMode)
{
	switch (uViewMode)
	{
	case VM_THUMBNAILS:
		return IDS_VIEW_THUMBNAILS;
		break;

	case VM_TILES:
		return IDS_VIEW_TILES;
		break;

	case VM_EXTRALARGEICONS:
		return IDS_VIEW_EXTRALARGEICONS;
		break;

	case VM_LARGEICONS:
		return IDS_VIEW_LARGEICONS;
		break;

	case VM_ICONS:
		return IDS_VIEW_MEDIUMICONS;
		break;

	case VM_SMALLICONS:
		return IDS_VIEW_SMALLICONS;
		break;

	case VM_LIST:
		return IDS_VIEW_LIST;
		break;

	case VM_DETAILS:
		return IDS_VIEW_DETAILS;
		break;
	}

	return -1;
}