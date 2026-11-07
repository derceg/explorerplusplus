// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ThemeManager.h"
#include "DarkModeManager.h"

ThemeManager::ThemeManager(const DarkModeManager *darkModeManager) :
	m_darkModeManager(darkModeManager)
{
}

const ColorProvider *ThemeManager::GetCurrentColorProvider() const
{
	if (m_darkModeManager->IsDarkModeEnabled())
	{
		return &m_darkModeColorProvider;
	}
	else
	{
		return &m_lightModeColorProvider;
	}
}

const DarkModeColorProvider *ThemeManager::GetDarkModeColorProvider() const
{
	return &m_darkModeColorProvider;
}
