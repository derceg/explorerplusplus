// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ThemeWindowTracker.h"
#include "ThemeManager.h"

ThemeWindowTracker::ThemeWindowTracker(HWND hwnd, ThemeManager *themeManager) :
	m_hwnd(hwnd),
	m_themeManager(themeManager)
{
	m_themeManager->TrackTopLevelWindow(m_hwnd);
}

ThemeWindowTracker::~ThemeWindowTracker()
{
	m_themeManager->UntrackTopLevelWindow(m_hwnd);
}
