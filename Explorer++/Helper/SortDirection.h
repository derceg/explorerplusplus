// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BetterEnumsWrapper.h"

// clang-format off
BETTER_ENUM(SortDirection, int,
	Ascending = 0,
	Descending = 1
)
// clang-format on

constexpr SortDirection InvertSortDirection(SortDirection direction)
{
	return direction == +SortDirection::Ascending ? SortDirection::Descending
												  : SortDirection::Ascending;
}
