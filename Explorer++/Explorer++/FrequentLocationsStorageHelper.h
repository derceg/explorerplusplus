// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <chrono>

namespace FrequentLocationsStorageHelper
{

// Since C++20, the epoch used by system_clock is fixed (to the Unix epoch). However, the period of
// the clock is implementation defined. By storing times in a fixed unit, it's guaranteed that
// saving/loading data will work, even if the underlying implementation changes the tick duration.
// As this value is used when saving/loading visit time data, it shouldn't be changed.
using StorageDurationType = std::chrono::microseconds;

inline constexpr int MAX_ITEMS_TO_STORE = 50;

}
