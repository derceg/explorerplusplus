#pragma once

HWND CreateListView(HWND hParent, DWORD dwStyle);
HWND CreateTreeView(HWND hParent, DWORD dwStyle);
HWND CreateStatusBar(HWND hParent, DWORD dwStyle);
HWND CreateToolbar(HWND hParent, DWORD dwStyle, DWORD dwExStyle);
HWND CreateComboBox(HWND Parent, DWORD dwStyle);
HWND CreateTabControl(HWND hParent, DWORD dwStyle);
BOOL PinStatusBar(HWND hStatusBar, int Width, int Height);
BOOL AddPathsToComboBoxEx(HWND hComboBoxEx, const TCHAR *Path);

/* Dialog. */
BOOL lCheckDlgButton(HWND hDlg, int ButtonId, BOOL bCheck);

/* Toolbar/Rebar. */
void AddStyleToToolbar(UINT *fStyle, UINT fStyleToAdd);
void AddGripperStyle(UINT *fStyle, BOOL bAddGripper);
void UpdateToolbarBandSizing(HWND hRebar, HWND hToolbar);