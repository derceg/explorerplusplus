// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ColorProvider.h"
#include <wil/resource.h>

class LightModeColorProvider : public ColorProvider
{
public:
	LightModeColorProvider();

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
	COLORREF GetTabBarBackgroundColor() const override;
	COLORREF GetTabBackgroundColor() const override;

	HBRUSH GetBackgroundBrush() const override;
	HBRUSH GetSelectedItemBackgroundBrush() const override;
	HBRUSH GetHotItemBackgroundBrush() const override;
	HBRUSH GetBorderBrush() const override;
	HBRUSH GetComboBoxExBackgroundBrush() const override;
	HBRUSH GetTabBarBackgroundBrush() const override;
	HBRUSH GetTabBackgroundBrush() const override;

private:
	const wil::unique_hbrush m_backgroundBrush;
	const wil::unique_hbrush m_selectedItemBackgroundBrush;
	const wil::unique_hbrush m_hotItemBackgroundBrush;
	const wil::unique_hbrush m_borderBrush;
	const wil::unique_hbrush m_comboBoxExBackgroundBrush;
	const wil::unique_hbrush m_tabBarBackgroundBrush;
	const wil::unique_hbrush m_tabBackgroundBrush;
};
