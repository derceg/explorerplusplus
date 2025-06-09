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

	// This will be called by the view when an icon may no longer be required (e.g. when the icon
	// for a tab changes, or a tab is removed). If the method returns true, it indicates that the
	// specified icon should be removed from the view's image list.
	virtual bool ShouldRemoveIcon(int iconIndex) = 0;
};
