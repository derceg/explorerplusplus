// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "WindowHelper.h"

/* Centers one window (hChild) with respect to
another (hParent), as per the Windows UX
Guidelines (2009).
This means placing the child window 45% of the
way from the top of the parent window (with 55%
of the space left between the bottom of the
child window and the bottom of the parent window).*/
BOOL CenterWindow(HWND hParent, HWND hChild)
{
	RECT rcParent;
	BOOL bRet = GetClientRect(hParent, &rcParent);

	if (!bRet)
	{
		return FALSE;
	}

	RECT rcChild;
	bRet = GetClientRect(hChild, &rcChild);

	if (!bRet)
	{
		return FALSE;
	}

	/* Take the offset between the two windows,
	and map it back to the desktop. */
	POINT ptOrigin;
	ptOrigin.x = (GetRectWidth(&rcParent) - GetRectWidth(&rcChild)) / 2;
	ptOrigin.y = static_cast<LONG>((GetRectHeight(&rcParent) - GetRectHeight(&rcChild)) * 0.45);

	SetLastError(0);
	int iRet = MapWindowPoints(hParent, HWND_DESKTOP, &ptOrigin, 1);

	if (iRet == 0 && GetLastError() != 0)
	{
		return FALSE;
	}

	return SetWindowPos(hChild, nullptr, ptOrigin.x, ptOrigin.y, 0, 0,
		SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOZORDER);
}

std::wstring GetDlgItemString(HWND dlg, int controlId)
{
	HWND control = GetDlgItem(dlg, controlId);

	if (!control)
	{
		return {};
	}

	return GetWindowString(control);
}

std::wstring GetWindowString(HWND hwnd)
{
	int textLength = GetWindowTextLength(hwnd);

	if (textLength == 0)
	{
		return {};
	}

	// Add space for the terminating NULL.
	textLength += 1;

	std::wstring text;
	text.resize(textLength);

	GetWindowText(hwnd, text.data(), textLength);

	// GetWindowText will copy the entire string, along with the terminating NULL. If the buffer
	// weren't reduced in size here, the terminating NULL would then end up as part of the actual
	// string, which is undesirable.
	text.resize(textLength - 1);

	return text;
}

BOOL lShowWindow(HWND hwnd, BOOL bShowWindow)
{
	int windowShowState;

	if (bShowWindow)
	{
		windowShowState = SW_SHOW;
	}
	else
	{
		windowShowState = SW_HIDE;
	}

	return ShowWindow(hwnd, windowShowState);
}

BOOL AddWindowStyle(HWND hwnd, UINT fStyle, BOOL bAdd)
{
	LONG_PTR fCurrentStyle = GetWindowLongPtr(hwnd, GWL_STYLE);

	if (fCurrentStyle == 0)
	{
		return FALSE;
	}

	if (bAdd)
	{
		/* Only add the style if it isn't already present. */
		if ((fCurrentStyle & fStyle) != fStyle)
		{
			fCurrentStyle |= fStyle;
		}
	}
	else
	{
		/* Only remove the style if it is present. */
		if ((fCurrentStyle & fStyle) == fStyle)
		{
			fCurrentStyle &= ~static_cast<LONG_PTR>(fStyle);
		}
	}

	/* See the documentation for SetWindowLongPtr
	for an explanation of why this is necessary. */
	SetLastError(0);
	LONG_PTR lRet = SetWindowLongPtr(hwnd, GWL_STYLE, fCurrentStyle);
	return (lRet == 0) && (GetLastError() != 0);
}

int GetRectHeight(const RECT *rc)
{
	return rc->bottom - rc->top;
}

int GetRectWidth(const RECT *rc)
{
	return rc->right - rc->left;
}

bool BringWindowToForeground(HWND wnd)
{
	if (IsIconic(wnd))
	{
		// The window is minimized, so this will restore it to its previous position. The window
		// will be maximized if it was previously maximized and restored to its normal position
		// otherwise.
		ShowWindow(wnd, SW_RESTORE);
	}

	return SetForegroundWindow(wnd);
}

bool operator==(const RECT &first, const RECT &second)
{
	return EqualRect(&first, &second);
}
