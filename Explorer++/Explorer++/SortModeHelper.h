// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/SortModes.h"

UINT GetSortMenuItemStringIndex(UINT uItemId);
int DetermineSortModeMenuId(SortMode sortMode);
int DetermineGroupModeMenuId(SortMode sortMode);