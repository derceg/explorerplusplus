#pragma once

typedef enum
{
	VM_ICONS = 1,
	VM_SMALLICONS = 2,
	VM_LIST = 3,
	VM_DETAILS = 4,
	VM_TILES = 5,
	VM_THUMBNAILS = 6,
	VM_EXTRALARGEICONS = 7,
	VM_LARGEICONS = 8,
} VIEW_MODES;

int GetViewModeMenuId(UINT uViewMode);
int GetViewModeMenuStringId(UINT uViewMode);