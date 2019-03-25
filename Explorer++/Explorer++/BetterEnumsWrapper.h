// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#define BETTER_ENUMS_DEFAULT_CONSTRUCTOR(Enum) \
  public:                                      \
    Enum() = default;

#pragma warning(push)
#pragma warning(disable:4100) //unreferenced formal parameter
#include "../ThirdParty/BetterEnums/enum.h"
#pragma warning(pop)