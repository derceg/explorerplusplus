// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#define BETTER_ENUMS_DEFAULT_CONSTRUCTOR(Enum)                                                     \
public:                                                                                            \
	Enum() = default;

#include <better-enums/enum.h>

// Provides a basic way to detect whether a type is a Better Enums enum. Based on the fact that
// _to_index() is a method provided by Better Enums and unlikely to appear elsewhere.
// clang-format off
template <typename T>
concept BetterEnum = requires(T item)
{
	{ item._to_index() } -> std::integral;
};
// clang-format on
