// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

// This class automatically tracks/untracks a window with the ThemeManager class. It should only be
// used with top-level windows (e.g. the main window or a dialog).
class ThemeWindowTracker
{
public:
	ThemeWindowTracker(HWND hwnd);
	~ThemeWindowTracker();

private:
	HWND m_hwnd;
};
