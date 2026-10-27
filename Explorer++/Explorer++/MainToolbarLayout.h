// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MainToolbarButtonId.h"
#include <vector>

// Represents a ordered set of toolbar buttons and separators. The advantage this has over a raw
// vector is that it will only allow non-separator items to be added a single time.
class MainToolbarLayout
{
public:
	MainToolbarLayout() = default;
	MainToolbarLayout(const std::vector<MainToolbarButtonId> &buttons);

	void AddButton(MainToolbarButtonId button);
	const std::vector<MainToolbarButtonId> &GetButtons() const;

	// This is only used in tests.
	bool operator==(const MainToolbarLayout &) const = default;

private:
	std::vector<MainToolbarButtonId> m_buttons;
};
