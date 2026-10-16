// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <functional>

struct MouseEvent
{
	MouseEvent(POINT ptScreen, bool shiftKey, bool ctrlKey) :
		ptScreen(ptScreen),
		shiftKey(shiftKey),
		ctrlKey(ctrlKey)
	{
	}

	const POINT ptScreen;
	const bool shiftKey;
	const bool ctrlKey;

	bool operator==(const MouseEvent &other) const
	{
		return ptScreen.x == other.ptScreen.x && ptScreen.y == other.ptScreen.y
			&& shiftKey == other.shiftKey && ctrlKey == other.ctrlKey;
	}
};

using MouseEventCallback = std::function<void(const MouseEvent &event)>;
