// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

// The CustomGripper control uses the appropriate functions provided by Windows to draw the size
// grip shown in the bottom right corner of a window.
namespace CustomGripper
{
	inline const TCHAR CLASS_NAME[] = L"CustomGripper";

	void Initialize(HWND mainWindow, COLORREF backgroundColor);
	SIZE GetDpiScaledSize(HWND parentWindow);
}