#pragma once

#include <windows.h>

void	AddPathsToComboBoxEx(HWND,TCHAR *);
HWND	CreateComboBox(HWND,DWORD);
HWND	CreateListView(HWND,DWORD);
HWND	CreateStatusBar(HWND,DWORD);
HWND	CreateTabControl(HWND,UINT Style);
HWND	CreateToolbar(HWND hParent,DWORD dwStyle,DWORD dwExStyle);
HWND	CreateTreeView(HWND,DWORD);
int		ResizeStatusBar(HWND,int,int);