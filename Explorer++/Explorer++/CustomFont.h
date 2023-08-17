// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Literals.h"
#include <string>

class CustomFont
{
public:
	static constexpr int MINIMUM_SIZE = 6_pt;
	static constexpr int MAXIMUM_SIZE = 24_pt;

	CustomFont(const std::wstring &name, int size);

	std::wstring GetName() const;
	int GetSize() const;

	// This will be used to detect whether or not the font selected by the user is different from
	// the current custom font.
	bool operator==(const CustomFont &) const = default;

private:
	std::wstring m_name;

	// The size, in points, of this font.
	int m_size;
};
