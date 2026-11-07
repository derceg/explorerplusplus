// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ThemeWindowTracker.h"
#include "WindowThemeManager.h"

ThemeWindowTracker::ThemeWindowTracker(HWND hwnd, WindowThemeManager *windowThemeManager) :
	m_hwnd(hwnd),
	m_windowThemeManager(windowThemeManager)
{
	m_windowThemeManager->TrackTopLevelWindow(m_hwnd);
}

ThemeWindowTracker::~ThemeWindowTracker()
{
	m_windowThemeManager->UntrackTopLevelWindow(m_hwnd);
}
