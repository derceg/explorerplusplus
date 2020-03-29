// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

HWND CreateListView(HWND hParent, DWORD dwStyle);
HWND CreateTreeView(HWND hParent, DWORD dwStyle);
HWND CreateStatusBar(HWND hParent, DWORD dwStyle);
HWND CreateToolbar(HWND hParent, DWORD dwStyle, DWORD dwExStyle);
HWND CreateComboBox(HWND parent, DWORD dwStyle);
HWND CreateTabControl(HWND hParent, DWORD dwStyle);
HWND CreateTooltipControl(HWND parent, HINSTANCE instance);
BOOL PinStatusBar(HWND hStatusBar, int width, int height);
BOOL AddPathsToComboBoxEx(HWND hComboBoxEx, const TCHAR *path);

/* Dialog. */
BOOL lCheckDlgButton(HWND hDlg, int buttonId, BOOL bCheck);

/* Toolbar/Rebar. */
void AddStyleToToolbar(UINT *fStyle, UINT fStyleToAdd);
void AddGripperStyle(UINT *fStyle, BOOL bAddGripper);
void UpdateToolbarBandSizing(HWND hRebar, HWND hToolbar);