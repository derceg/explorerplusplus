#pragma once

#define WM_USER_HOLDERRESIZED		WM_APP + 300

HWND	CreateHolderWindow(HWND hParent,TCHAR *szWindowName,UINT uStyle);