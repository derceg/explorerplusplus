// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ThemeWindowTracker.h"
#include "ThemeManager.h"

ThemeWindowTracker::ThemeWindowTracker(HWND hwnd) : m_hwnd(hwnd)
{
	ThemeManager::GetInstance().TrackTopLevelWindow(m_hwnd);
}

ThemeWindowTracker::~ThemeWindowTracker()
{
	ThemeManager::GetInstance().UntrackTopLevelWindow(m_hwnd);
}
