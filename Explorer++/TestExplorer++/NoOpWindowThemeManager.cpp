// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "NoOpWindowThemeManager.h"

void NoOpWindowThemeManager::TrackTopLevelWindow(HWND hwnd)
{
	UNREFERENCED_PARAMETER(hwnd);
}

void NoOpWindowThemeManager::UntrackTopLevelWindow(HWND hwnd)
{
	UNREFERENCED_PARAMETER(hwnd);
}

void NoOpWindowThemeManager::ApplyThemeToWindowAndChildren(HWND hwnd)
{
	UNREFERENCED_PARAMETER(hwnd);
}
