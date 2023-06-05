// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "WinUserBackwardsCompatibility.h"
#include <wil/resource.h>

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

private:
	DpiCompatibility();

	wil::unique_hmodule m_user32;

	decltype(&::SystemParametersInfoForDpi) m_SystemParametersInfoForDpi;
	decltype(&::GetSystemMetricsForDpi) m_GetSystemMetricsForDpi;
	decltype(&::GetDpiForWindow) m_GetDpiForWindow;
};
