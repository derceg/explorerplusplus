// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <windows.h>

class ColorProvider
{
public:
	virtual ~ColorProvider() = default;

	virtual COLORREF GetBackgroundColor() const = 0;
	virtual COLORREF GetTextColor() const = 0;

	// The text color used for items that are active, but considered to be in the background. For
	// example, in a tab control, a single item is selected. The text for the other tabs could be
	// drawn in this color to visually distinguish them from the selected tab.
	virtual COLORREF GetBackgroundTextColor() const = 0;

	virtual COLORREF GetDisabledTextColor() const = 0;

	// The color of foreground elements (e.g. the toolbar insertion mark).
	virtual COLORREF GetForegroundColor() const = 0;

	// The background color of the selected item.
	virtual COLORREF GetSelectedItemBackgroundColor() const = 0;

	// The background color of the hot item (i.e. the item that's under the mouse).
	virtual COLORREF GetHotItemBackgroundColor() const = 0;

	// The background color of a checked toolbar item.
	virtual COLORREF GetToolbarCheckedBackgroundColor() const = 0;

	// The color of borders (e.g. the borders around an individual tab).
	virtual COLORREF GetBorderColor() const = 0;

	virtual COLORREF GetComboBoxExBackgroundColor() const = 0;

	// The background color of each non-selected tab in a tab control.
	virtual COLORREF GetTabBackgroundColor() const = 0;

	virtual HBRUSH GetBackgroundBrush() const = 0;
	virtual HBRUSH GetSelectedItemBackgroundBrush() const = 0;
	virtual HBRUSH GetHotItemBackgroundBrush() const = 0;
	virtual HBRUSH GetBorderBrush() const = 0;
	virtual HBRUSH GetComboBoxExBackgroundBrush() const = 0;
	virtual HBRUSH GetTabBackgroundBrush() const = 0;
};
