// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

// The declarations below are copied from WinUser.h. Because these functions
// don't exist in the minimum version of Windows supported by this application
// (currently Windows 7), the declarations have been copied below. This makes it
// easier to call the functions dynamically.
// 
// Grouping functions together in a single header (that matches the original
// header) also makes it easier to see which functions from later versions of
// Windows are being used.
// 
// See the official documentation for information on the minimum version of
// Windows required to call these functions directly.

BOOL
WINAPI
SystemParametersInfoForDpi(
	_In_ UINT uiAction,
	_In_ UINT uiParam,
	_Pre_maybenull_ _Post_valid_ PVOID pvParam,
	_In_ UINT fWinIni,
	_In_ UINT dpi);

int
WINAPI
GetSystemMetricsForDpi(
	_In_ int nIndex,
	_In_ UINT dpi);

UINT
WINAPI
GetDpiForWindow(
	_In_ HWND hwnd);