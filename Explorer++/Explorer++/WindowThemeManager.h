// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class WindowThemeManager
{
public:
	virtual ~WindowThemeManager() = default;

	virtual void TrackTopLevelWindow(HWND hwnd) = 0;
	virtual void UntrackTopLevelWindow(HWND hwnd) = 0;
	virtual void ApplyThemeToWindowAndChildren(HWND hwnd) = 0;
};
