// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "WindowThemeManager.h"

class NoOpWindowThemeManager : public WindowThemeManager
{
public:
	// WindowThemeManager
	void TrackTopLevelWindow(HWND hwnd) override;
	void UntrackTopLevelWindow(HWND hwnd) override;
	void ApplyThemeToWindowAndChildren(HWND hwnd) override;
};
