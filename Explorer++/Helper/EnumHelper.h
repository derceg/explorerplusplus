// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <type_traits>

template <typename T>
concept Enum = std::is_enum_v<T>;

// TODO: This can be replaced with std::to_underlying() once C++23 support is available.
template <Enum T>
constexpr std::underlying_type_t<T> ToUnderlying(T value)
{
	return static_cast<std::underlying_type_t<T>>(value);
}
