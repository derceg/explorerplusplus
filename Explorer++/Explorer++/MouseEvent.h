// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <functional>

struct MouseEvent
{
	MouseEvent(POINT ptClient, bool shiftKey, bool ctrlKey) :
		ptClient(ptClient),
		shiftKey(shiftKey),
		ctrlKey(ctrlKey)
	{
	}

	const POINT ptClient;
	const bool shiftKey;
	const bool ctrlKey;

	bool operator==(const MouseEvent &other) const
	{
		return ptClient.x == other.ptClient.x && ptClient.y == other.ptClient.y
			&& shiftKey == other.shiftKey && ctrlKey == other.ctrlKey;
	}
};

using MouseEventCallback = std::function<void(const MouseEvent &event)>;
