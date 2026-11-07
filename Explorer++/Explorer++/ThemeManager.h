// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DarkModeColorProvider.h"
#include "LightModeColorProvider.h"

class DarkModeManager;

class ThemeManager
{
public:
	ThemeManager(const DarkModeManager *darkModeManager);

	const ColorProvider *GetCurrentColorProvider() const;
	const DarkModeColorProvider *GetDarkModeColorProvider() const;

private:
	const DarkModeManager *const m_darkModeManager;

	DarkModeColorProvider m_darkModeColorProvider;
	LightModeColorProvider m_lightModeColorProvider;
};
