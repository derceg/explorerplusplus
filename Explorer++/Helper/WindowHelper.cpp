/******************************************************************
*
* Project: Helper
* File: WindowHelper.cpp
* License: GPL - See COPYING in the top level directory
*
* Window helper functionality.
*
* Written by David Erceg
* www.explorerplusplus.com
*
*****************************************************************/

#include "stdafx.h"
#include "WindowHelper.h"


/* Centers one window (hChild) with respect to
another (hParent), as per the Windows UX
Guidelines (2009).
This means placing the child window 45% of the
way from the top of the parent window (with 55%
of the space left between the bottom of the
child window and the bottom of the parent window).*/
void CenterWindow(HWND hParent, HWND hChild)
{
	RECT rcParent;
	RECT rcChild;
	POINT ptOrigin;

	GetClientRect(hParent, &rcParent);
	GetClientRect(hChild, &rcChild);

	/* Take the offset between the two windows, and map it back to the
	desktop. */
	ptOrigin.x = (GetRectWidth(&rcParent) - GetRectWidth(&rcChild)) / 2;
	ptOrigin.y = (LONG) ((GetRectHeight(&rcParent) - GetRectHeight(&rcChild)) * 0.45);
	MapWindowPoints(hParent, HWND_DESKTOP, &ptOrigin, 1);

	SetWindowPos(hChild, NULL, ptOrigin.x, ptOrigin.y,
		0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOZORDER);
}

void GetWindowString(HWND hwnd, std::wstring &str)
{
	int iLen = GetWindowTextLength(hwnd);

	TCHAR *szTemp = new TCHAR[iLen + 1];
	GetWindowText(hwnd, szTemp, iLen + 1);

	str = szTemp;

	delete[] szTemp;
}

BOOL lShowWindow(HWND hwnd, BOOL bShowWindow)
{
	int WindowShowState;

	if(bShowWindow)
		WindowShowState = SW_SHOW;
	else
		WindowShowState = SW_HIDE;

	return ShowWindow(hwnd, WindowShowState);
}

void AddWindowStyle(HWND hwnd, UINT fStyle, BOOL bAdd)
{
	LONG_PTR fCurrentStyle;

	fCurrentStyle = GetWindowLongPtr(hwnd, GWL_STYLE);

	if(bAdd)
	{
		/* Only add the style if it isn't already present. */
		if((fCurrentStyle & fStyle) != fStyle)
			fCurrentStyle |= fStyle;
	}
	else
	{
		/* Only remove the style if it is present. */
		if((fCurrentStyle & fStyle) == fStyle)
			fCurrentStyle &= ~fStyle;
	}

	SetWindowLongPtr(hwnd, GWL_STYLE, fCurrentStyle);
}

int GetRectHeight(const RECT *rc)
{
	return rc->bottom - rc->top;
}

int GetRectWidth(const RECT *rc)
{
	return rc->right - rc->left;
}