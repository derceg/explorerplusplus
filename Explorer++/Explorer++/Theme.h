// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BetterEnumsWrapper.h"
#include <string>

// clang-format off
BETTER_ENUM(Theme, int,
	Light = 0,
	Dark = 1,
	System = 2
)
// clang-format on

std::wstring GetThemeText(Theme theme);
