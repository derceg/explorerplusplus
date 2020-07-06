// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Manages the 'holder window'. This window acts as a generic
 * container for child windows.
 */

#include "stdafx.h"
#include "HolderWindow.h"
#include "DarkModeHelper.h"
#include "HolderWindowInternal.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/Macros.h"

#define FOLDERS_TEXT_X 5
#define FOLDERS_TEXT_Y 2

#define HOLDER_CLASS_NAME _T("Holder")

ATOM RegisterHolderWindowClass();
LRESULT CALLBACK HolderWndProcStub(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

HolderWindow::HolderWindow(HWND hHolder)
{
	m_hHolder = hHolder;
	m_bHolderResizing = FALSE;
}

ATOM RegisterHolderWindowClass()
{
	WNDCLASS wc;

	wc.style = 0;
	wc.lpfnWndProc = HolderWndProcStub;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = sizeof(HolderWindow *);
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hIcon = nullptr;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (darkModeHelper.IsDarkModeEnabled())
	{
		wc.hbrBackground = darkModeHelper.GetBackgroundBrush();
	}
	else
	{
		wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
	}

	wc.lpszMenuName = nullptr;
	wc.lpszClassName = HOLDER_CLASS_NAME;

	return RegisterClass(&wc);
}

HWND CreateHolderWindow(HWND hParent, TCHAR *szWindowName, UINT uStyle)
{
	HWND hHolder;

	RegisterHolderWindowClass();

	hHolder = CreateWindowEx(0, HOLDER_CLASS_NAME, szWindowName, uStyle, 0, 0, 0, 0, hParent,
		nullptr, GetModuleHandle(nullptr), nullptr);

	return hHolder;
}

LRESULT CALLBACK HolderWndProcStub(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	auto *pHolderWindow = (HolderWindow *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (msg)
	{
	case WM_CREATE:
	{
		pHolderWindow = new HolderWindow(hwnd);

		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) pHolderWindow);
	}
	break;

	case WM_NCDESTROY:
		delete pHolderWindow;
		return 0;
	}

	return pHolderWindow->HolderWndProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK HolderWindow::HolderWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_LBUTTONDOWN:
		OnHolderWindowLButtonDown(lParam);
		break;

	case WM_LBUTTONUP:
		OnHolderWindowLButtonUp();
		break;

	case WM_MOUSEMOVE:
		return OnHolderWindowMouseMove(lParam);

	case WM_PAINT:
		OnHolderWindowPaint(hwnd);
		break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

/*
 * Draws the window text onto the holder
 * window.
 */
void HolderWindow::OnHolderWindowPaint(HWND hwnd)
{
	PAINTSTRUCT ps;
	NONCLIENTMETRICS ncm;
	HDC hdc;
	HFONT hFont;
	RECT rc;
	TCHAR szHeader[64];

	GetClientRect(hwnd, &rc);

	hdc = BeginPaint(hwnd, &ps);

	auto &dpiCompat = DpiCompatibility::GetInstance();
	UINT dpi = dpiCompat.GetDpiForWindow(hwnd);

	ncm.cbSize = sizeof(ncm);
	dpiCompat.SystemParametersInfoForDpi(
		SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0, dpi);
	ncm.lfSmCaptionFont.lfWeight = FW_NORMAL;
	hFont = CreateFontIndirect(&ncm.lfSmCaptionFont);

	SelectObject(hdc, hFont);

	GetWindowText(hwnd, szHeader, SIZEOF_ARRAY(szHeader));

	SetBkMode(hdc, TRANSPARENT);

	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (darkModeHelper.IsDarkModeEnabled())
	{
		SetTextColor(hdc, DarkModeHelper::FOREGROUND_COLOR);
	}

	TextOut(hdc, FOLDERS_TEXT_X, FOLDERS_TEXT_Y, szHeader, lstrlen(szHeader));

	DeleteObject(hFont);

	EndPaint(hwnd, &ps);
}

void HolderWindow::OnHolderWindowLButtonDown(LPARAM lParam)
{
	POINTS cursorPos;
	RECT rc;

	cursorPos = MAKEPOINTS(lParam);
	GetClientRect(m_hHolder, &rc);

	if (cursorPos.x >= (rc.right - 10))
	{
		SetCursor(LoadCursor(nullptr, IDC_SIZEWE));

		m_bHolderResizing = TRUE;

		SetFocus(m_hHolder);
		SetCapture(m_hHolder);
	}
}

void HolderWindow::OnHolderWindowLButtonUp()
{
	m_bHolderResizing = FALSE;

	ReleaseCapture();
}

int HolderWindow::OnHolderWindowMouseMove(LPARAM lParam)
{
	static POINTS ptsPrevCursor;
	POINTS ptsCursor;
	RECT rc;

	ptsCursor = MAKEPOINTS(lParam);
	GetClientRect(m_hHolder, &rc);

	/* Is the window in the process of been resized? */
	if (m_bHolderResizing)
	{
		/* Mouse hasn't moved. */
		if ((ptsPrevCursor.x == ptsCursor.x) && (ptsPrevCursor.y == ptsCursor.y))
		{
			return 0;
		}

		ptsPrevCursor.x = ptsCursor.x;
		ptsPrevCursor.y = ptsCursor.y;

		SendMessage(
			GetParent(m_hHolder), WM_USER_HOLDERRESIZED, (WPARAM) m_hHolder, (LPARAM) ptsCursor.x);

		return 1;
	}

	if (ptsCursor.x >= (rc.right - 10))
	{
		SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
	}

	return 0;
}