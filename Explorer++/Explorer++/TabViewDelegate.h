// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

// Allows the TabView controller to be notified of events that occur within the view.
class TabViewDelegate
{
public:
	virtual ~TabViewDelegate() = default;

	virtual void OnTabMoved(int fromIndex, int toIndex) = 0;
};
