// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class ThemeManager;

// This class automatically tracks/untracks a window with the ThemeManager class. It should only be
// used with top-level windows (e.g. the main window or a dialog).
class ThemeWindowTracker
{
public:
	ThemeWindowTracker(HWND hwnd, ThemeManager *themeManager);
	~ThemeWindowTracker();

private:
	const HWND m_hwnd;
	ThemeManager *const m_themeManager;
};
