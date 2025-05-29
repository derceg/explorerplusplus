// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/SortModes.h"

bool IsSortModeMenuItemId(UINT menuItemId);
SortMode GetSortModeForMenuItemId(UINT menuItemId);
UINT GetMenuItemIdForSortMode(SortMode sortMode);
