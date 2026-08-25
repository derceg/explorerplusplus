// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <string>

BOOL CenterWindow(HWND hParent, HWND hChild);
std::wstring GetDlgItemString(HWND dlg, int controlId);
std::wstring GetWindowString(HWND hwnd);
BOOL lShowWindow(HWND hwnd, BOOL bShowWindow);
bool HasWindowStyles(HWND hwnd, LONG_PTR styles);
void SetWindowStyles(HWND hwnd, LONG_PTR styles, bool add);
int GetRectHeight(const RECT *rc);
int GetRectWidth(const RECT *rc);
bool BringWindowToForeground(HWND wnd);
bool IsRectVisible(const RECT *rect);
void RecalcWindowCursor(HWND window);
bool IsHighContrastEnabled();

// This is only used in tests.
bool operator==(const RECT &first, const RECT &second);
