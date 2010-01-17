#ifndef HOLDERWINDOW_INCLUDED
#define HOLDERWINDOW_INCLUDED

#define WM_USER_HOLDERRESIZED		WM_USER + 300

HWND	CreateHolderWindow(HWND hParent,TCHAR *szWindowName,UINT uStyle);

#endif