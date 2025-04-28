// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>

class DarkModeColorProvider
{
public:
	static constexpr COLORREF BACKGROUND_COLOR = RGB(32, 32, 32);
	static constexpr COLORREF TEXT_COLOR = RGB(255, 255, 255);

	// The text color used for items that are active, but considered to be in the background. For
	// example, in a tab control, a single item is selected. The text for the other tabs could be
	// drawn in this color to visually distinguish them from the selected tab.
	static constexpr COLORREF TEXT_COLOR_BACKGROUND = RGB(180, 180, 180);

	static constexpr COLORREF TEXT_COLOR_DISABLED = RGB(121, 121, 121);

	// The color of foreground elements (e.g. the toolbar insertion mark).
	static constexpr COLORREF FOREGROUND_COLOR = RGB(255, 255, 255);

	// The background color of the selected item.
	static constexpr COLORREF SELECTED_ITEM_BACKGROUND_COLOR = RGB(100, 100, 100);

	// The background color of the hot item (i.e. the item that's under the mouse).
	static constexpr COLORREF HOT_ITEM_BACKGROUND_COLOR = RGB(71, 71, 71);

	// The background color of a checked toolbar item.
	static constexpr COLORREF TOOLBAR_CHECKED_BACKGROUND_COLOR = RGB(80, 80, 80);

	// The color of borders (e.g. the borders around an individual tab).
	static constexpr COLORREF BORDER_COLOR = RGB(120, 120, 120);

	// This is the same background color as used in the Explorer address bar.
	static constexpr COLORREF COMBO_BOX_EX_BACKGROUND_COLOR = RGB(25, 25, 25);

	// This is the background color of each non-selected tab in a tab control.
	static constexpr COLORREF TAB_BACKGROUND_COLOR = RGB(38, 38, 38);

	DarkModeColorProvider();

	HBRUSH GetBackgroundBrush() const;
	HBRUSH GetSelectedItemBackgroundBrush() const;
	HBRUSH GetHotItemBackgroundBrush() const;
	HBRUSH GetBorderBrush() const;
	HBRUSH GetComboBoxExBackgroundBrush() const;
	HBRUSH GetTabBackgroundBrush() const;

private:
	const wil::unique_hbrush m_backgroundBrush;
	const wil::unique_hbrush m_selectedItemBackgroundBrush;
	const wil::unique_hbrush m_hotItemBackgroundBrush;
	const wil::unique_hbrush m_borderBrush;
	const wil::unique_hbrush m_comboBoxExBackgroundBrush;
	const wil::unique_hbrush m_tabBackgroundBrush;
};
