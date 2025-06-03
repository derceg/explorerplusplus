// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "WinUserBackwardsCompatibility.h"
#include <wil/resource.h>

// Both of these messages are available on Windows 10 and later and the Windows headers are set up
// so that the values are only defined when the appropriate minimum version is set. Defining the
// messages here allows the application to respond to them. If the minimum version is increased to
// at least _WIN32_WINNT_WIN10, there's no need to manually define these values and the declarations
// can be removed.
static_assert(WINVER < _WIN32_WINNT_WIN10);
#define WM_DPICHANGED_BEFOREPARENT 0x02E2
#define WM_DPICHANGED_AFTERPARENT 0x02E3

class DpiCompatibility
{
public:
	static DpiCompatibility &GetInstance();

	BOOL WINAPI SystemParametersInfoForDpi(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni,
		UINT dpi);
	int WINAPI GetSystemMetricsForDpi(int nIndex, UINT dpi);
	UINT WINAPI GetDpiForWindow(HWND hwnd);

	// Scales a value targeted at 96 DPI to the DPI of the specified window.
	int ScaleValue(HWND hwnd, int value);

	int PointsToPixels(HWND hwnd, int pt);
	int PixelsToPoints(HWND hwnd, int px);

private:
	using SystemParametersInfoForDpiType = BOOL(
		WINAPI *)(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni, UINT dpi);
	using GetSystemMetricsForDpiType = int(WINAPI *)(int nIndex, UINT dpi);
	using GetDpiForWindowType = UINT(WINAPI *)(HWND hwnd);

	DpiCompatibility();

	wil::unique_hmodule m_user32;

	SystemParametersInfoForDpiType m_SystemParametersInfoForDpi;
	GetSystemMetricsForDpiType m_GetSystemMetricsForDpi;
	GetDpiForWindowType m_GetDpiForWindow;
};
