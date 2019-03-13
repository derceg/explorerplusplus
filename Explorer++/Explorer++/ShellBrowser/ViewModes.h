// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

enum ViewMode
{
	VM_ICONS = 1,
	VM_SMALLICONS = 2,
	VM_LIST = 3,
	VM_DETAILS = 4,
	VM_TILES = 5,
	VM_THUMBNAILS = 6,
	VM_EXTRALARGEICONS = 7,
	VM_LARGEICONS = 8,
	FIRST = VM_ICONS,
	LAST = VM_LARGEICONS
};

int GetViewModeMenuId(ViewMode viewMode);
int GetViewModeMenuStringId(ViewMode viewMode);