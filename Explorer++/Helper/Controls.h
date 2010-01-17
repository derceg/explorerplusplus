#ifndef CONTROLS_INCLUDED
#define CONTROLS_INCLUDED

#include <windows.h>

extern void	AddPathsToComboBoxEx(HWND,TCHAR *);
extern HWND	CreateButton(HWND,DWORD);
extern HWND	CreateComboBox(HWND,DWORD);
extern HWND	CreateListView(HWND,DWORD);
extern HWND	CreateStatusBar(HWND,DWORD);
extern HWND	CreateTabControl(HWND,UINT Style);
extern HWND CreateToolbar(HWND hParent,DWORD dwStyle,DWORD dwExStyle);
extern HWND	CreateTreeView(HWND,DWORD);
extern BOOL	InitControlClasses(DWORD);
extern int	ResizeStatusBar(HWND,int,int);
extern void	SetComboBoxExTitleString(HWND CbEx,TCHAR *Path);
void ListView_AddRemoveExtendedStyle(HWND hListView,DWORD dwStyle,BOOL bAdd);

#endif