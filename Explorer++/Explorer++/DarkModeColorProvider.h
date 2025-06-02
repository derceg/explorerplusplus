// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ColorProvider.h"
#include <wil/resource.h>

class DarkModeColorProvider : public ColorProvider
{
public:
	DarkModeColorProvider();

	COLORREF GetBackgroundColor() const override;
	COLORREF GetTextColor() const override;
	COLORREF GetBackgroundTextColor() const override;
	COLORREF GetDisabledTextColor() const override;
	COLORREF GetForegroundColor() const override;
	COLORREF GetSelectedItemBackgroundColor() const override;
	COLORREF GetHotItemBackgroundColor() const override;
	COLORREF GetToolbarCheckedBackgroundColor() const override;
	COLORREF GetBorderColor() const override;
	COLORREF GetComboBoxExBackgroundColor() const override;
	COLORREF GetTabBackgroundColor() const override;

	HBRUSH GetBackgroundBrush() const override;
	HBRUSH GetSelectedItemBackgroundBrush() const override;
	HBRUSH GetHotItemBackgroundBrush() const override;
	HBRUSH GetBorderBrush() const override;
	HBRUSH GetComboBoxExBackgroundBrush() const override;
	HBRUSH GetTabBackgroundBrush() const override;

private:
	static constexpr COLORREF BACKGROUND_COLOR = RGB(32, 32, 32);
	static constexpr COLORREF TEXT_COLOR = RGB(255, 255, 255);
	static constexpr COLORREF BACKGROUND_TEXT_COLOR = RGB(180, 180, 180);
	static constexpr COLORREF DISABLED_TEXT_COLOR = RGB(121, 121, 121);

	static constexpr COLORREF FOREGROUND_COLOR = RGB(255, 255, 255);

	static constexpr COLORREF SELECTED_ITEM_BACKGROUND_COLOR = RGB(100, 100, 100);
	static constexpr COLORREF HOT_ITEM_BACKGROUND_COLOR = RGB(71, 71, 71);

	static constexpr COLORREF TOOLBAR_CHECKED_BACKGROUND_COLOR = RGB(80, 80, 80);

	static constexpr COLORREF BORDER_COLOR = RGB(120, 120, 120);

	// This is the same background color as used in the Explorer address bar.
	static constexpr COLORREF COMBO_BOX_EX_BACKGROUND_COLOR = RGB(25, 25, 25);

	static constexpr COLORREF TAB_BACKGROUND_COLOR = RGB(38, 38, 38);

	const wil::unique_hbrush m_backgroundBrush;
	const wil::unique_hbrush m_selectedItemBackgroundBrush;
	const wil::unique_hbrush m_hotItemBackgroundBrush;
	const wil::unique_hbrush m_borderBrush;
	const wil::unique_hbrush m_comboBoxExBackgroundBrush;
	const wil::unique_hbrush m_tabBackgroundBrush;
};
