// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Literals.h"
#include <algorithm>
#include <cassert>
#include <string>

struct CustomFont
{
	static constexpr int MINIMUM_SIZE = 6_pt;
	static constexpr int MAXIMUM_SIZE = 24_pt;

	std::wstring name;

	// The size, in points, of this font.
	int size;

	CustomFont(const std::wstring &name, int size) :
		name(name),
		size(std::clamp(size, MINIMUM_SIZE, MAXIMUM_SIZE))
	{
	}

	// This will be used to detect whether or not the font selected by the user is different from
	// the current custom font.
	bool operator==(const CustomFont &) const = default;
};
