// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DarkModeColorProvider.h"

DarkModeColorProvider::DarkModeColorProvider() :
	m_backgroundBrush(CreateSolidBrush(BACKGROUND_COLOR)),
	m_selectedItemBackgroundBrush(CreateSolidBrush(SELECTED_ITEM_BACKGROUND_COLOR)),
	m_hotItemBackgroundBrush(CreateSolidBrush(HOT_ITEM_BACKGROUND_COLOR)),
	m_borderBrush(CreateSolidBrush(BORDER_COLOR)),
	m_comboBoxExBackgroundBrush(CreateSolidBrush(COMBO_BOX_EX_BACKGROUND_COLOR)),
	m_tabBackgroundBrush(CreateSolidBrush(TAB_BACKGROUND_COLOR))
{
}

HBRUSH DarkModeColorProvider::GetBackgroundBrush() const
{
	return m_backgroundBrush.get();
}

HBRUSH DarkModeColorProvider::GetSelectedItemBackgroundBrush() const
{
	return m_selectedItemBackgroundBrush.get();
}

HBRUSH DarkModeColorProvider::GetHotItemBackgroundBrush() const
{
	return m_hotItemBackgroundBrush.get();
}

HBRUSH DarkModeColorProvider::GetBorderBrush() const
{
	return m_borderBrush.get();
}

HBRUSH DarkModeColorProvider::GetComboBoxExBackgroundBrush() const
{
	return m_comboBoxExBackgroundBrush.get();
}

HBRUSH DarkModeColorProvider::GetTabBackgroundBrush() const
{
	return m_tabBackgroundBrush.get();
}
