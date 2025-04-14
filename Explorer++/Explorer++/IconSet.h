// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BetterEnumsWrapper.h"

// Note that the values in this enumeration are used when saving/loading the icon set and should not
// be changed.
// clang-format off
BETTER_ENUM(IconSet, int,
	Color = 0,
	Windows10 = 1,
	FluentUi = 2
)
// clang-format on
