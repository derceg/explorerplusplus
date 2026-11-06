// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LightModeColorProvider.h"

LightModeColorProvider::LightModeColorProvider() :
	m_backgroundBrush(CreateSolidBrush(GetBackgroundColor())),
	m_selectedItemBackgroundBrush(CreateSolidBrush(GetSelectedItemBackgroundColor())),
	m_hotItemBackgroundBrush(CreateSolidBrush(GetHotItemBackgroundColor())),
	m_borderBrush(CreateSolidBrush(GetBorderColor())),
	m_comboBoxExBackgroundBrush(CreateSolidBrush(GetComboBoxExBackgroundColor())),
	m_tabBarBackgroundBrush(CreateSolidBrush(GetTabBarBackgroundColor())),
	m_tabBackgroundBrush(CreateSolidBrush(GetTabBackgroundColor()))
{
}

COLORREF LightModeColorProvider::GetBackgroundColor() const
{
	return GetSysColor(COLOR_WINDOW);
}

COLORREF LightModeColorProvider::GetTextColor() const
{
	return GetSysColor(COLOR_WINDOWTEXT);
}

COLORREF LightModeColorProvider::GetBackgroundTextColor() const
{
	return GetSysColor(COLOR_WINDOWTEXT);
}

COLORREF LightModeColorProvider::GetDisabledTextColor() const
{
	return GetSysColor(COLOR_GRAYTEXT);
}

COLORREF LightModeColorProvider::GetForegroundColor() const
{
	return GetSysColor(COLOR_HIGHLIGHT);
}

COLORREF LightModeColorProvider::GetSelectedItemBackgroundColor() const
{
	return RGB(249, 249, 249);
}

COLORREF LightModeColorProvider::GetHotItemBackgroundColor() const
{
	return RGB(237, 237, 237);
}

COLORREF LightModeColorProvider::GetToolbarCheckedBackgroundColor() const
{
	return RGB(204, 232, 255);
}

COLORREF LightModeColorProvider::GetBorderColor() const
{
	return RGB(229, 229, 229);
}

COLORREF LightModeColorProvider::GetComboBoxExBackgroundColor() const
{
	return GetSysColor(COLOR_WINDOW);
}

COLORREF LightModeColorProvider::GetTabBarBackgroundColor() const
{
	return RGB(240, 240, 240);
}

COLORREF LightModeColorProvider::GetTabBackgroundColor() const
{
	return RGB(243, 243, 243);
}

HBRUSH LightModeColorProvider::GetBackgroundBrush() const
{
	return m_backgroundBrush.get();
}

HBRUSH LightModeColorProvider::GetSelectedItemBackgroundBrush() const
{
	return m_selectedItemBackgroundBrush.get();
}

HBRUSH LightModeColorProvider::GetHotItemBackgroundBrush() const
{
	return m_hotItemBackgroundBrush.get();
}

HBRUSH LightModeColorProvider::GetBorderBrush() const
{
	return m_borderBrush.get();
}

HBRUSH LightModeColorProvider::GetComboBoxExBackgroundBrush() const
{
	return m_comboBoxExBackgroundBrush.get();
}

HBRUSH LightModeColorProvider::GetTabBarBackgroundBrush() const
{
	return m_tabBarBackgroundBrush.get();
}

HBRUSH LightModeColorProvider::GetTabBackgroundBrush() const
{
	return m_tabBackgroundBrush.get();
}
