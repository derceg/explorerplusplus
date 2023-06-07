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
#include "../Helper/WindowHelper.h"

#define FOLDERS_TEXT_X 5
#define FOLDERS_TEXT_Y 2

#define HOLDER_CLASS_NAME _T("Holder")

ATOM RegisterHolderWindowClass();
LRESULT CALLBACK HolderWndProcStub(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

HolderWindow::HolderWindow(HWND hHolder) :
	m_hwnd(hHolder),
	m_sizingCursor(LoadCursor(nullptr, IDC_SIZEWE))
{
	auto &dpiCompat = DpiCompatibility::GetInstance();
	UINT dpi = dpiCompat.GetDpiForWindow(m_hwnd);

	NONCLIENTMETRICS nonClientMetrics = {};
	nonClientMetrics.cbSize = sizeof(nonClientMetrics);
	[[maybe_unused]] auto res = dpiCompat.SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS,
		sizeof(NONCLIENTMETRICS), &nonClientMetrics, 0, dpi);
	assert(res);

	nonClientMetrics.lfSmCaptionFont.lfWeight = FW_NORMAL;
	m_font.reset(CreateFontIndirect(&nonClientMetrics.lfSmCaptionFont));
	assert(m_font);
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
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = HOLDER_CLASS_NAME;

	return RegisterClass(&wc);
}

HWND CreateHolderWindow(HWND hParent, TCHAR *szWindowName, UINT uStyle)
{
	RegisterHolderWindowClass();

	HWND hHolder = CreateWindowEx(WS_EX_CONTROLPARENT, HOLDER_CLASS_NAME, szWindowName, uStyle, 0,
		0, 0, 0, hParent, nullptr, GetModuleHandle(nullptr), nullptr);

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
		OnLButtonDown({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
		break;

	case WM_LBUTTONUP:
		OnLButtonUp();
		break;

	case WM_MOUSEMOVE:
		return OnMouseMove({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });

	case WM_SETCURSOR:
		if (OnSetCursor(reinterpret_cast<HWND>(wParam)))
		{
			return TRUE;
		}
		break;

	case WM_ERASEBKGND:
		OnEraseBackground(reinterpret_cast<HDC>(wParam));
		return 1;

	case WM_PAINT:
		OnPaint(hwnd);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void HolderWindow::OnEraseBackground(HDC hdc)
{
	auto &darkModeHelper = DarkModeHelper::GetInstance();
	HBRUSH brush;

	if (darkModeHelper.IsDarkModeEnabled())
	{
		brush = darkModeHelper.GetBackgroundBrush();
	}
	else
	{
		brush = GetSysColorBrush(COLOR_BTNFACE);
	}

	RECT rc;
	GetClientRect(m_hwnd, &rc);
	FillRect(hdc, &rc, brush);
}

void HolderWindow::OnPaint(HWND hwnd)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);

	wil::unique_select_object object(SelectObject(hdc, m_font.get()));

	std::wstring header = GetWindowString(hwnd);

	SetBkMode(hdc, TRANSPARENT);

	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (darkModeHelper.IsDarkModeEnabled())
	{
		SetTextColor(hdc, DarkModeHelper::TEXT_COLOR);
	}

	TextOut(hdc, FOLDERS_TEXT_X, FOLDERS_TEXT_Y, header.c_str(), static_cast<int>(header.size()));

	EndPaint(hwnd, &ps);
}

void HolderWindow::OnLButtonDown(const POINT &pt)
{
	if (IsCursorInResizeStartRange(pt))
	{
		SetCursor(m_sizingCursor);

		m_resizing = true;

		RECT clientRect;
		GetClientRect(m_hwnd, &clientRect);
		m_resizeDistanceToEdge = clientRect.right - pt.x;

		SetCapture(m_hwnd);
	}
}

void HolderWindow::OnLButtonUp()
{
	m_resizing = false;
	m_resizeDistanceToEdge.reset();

	ReleaseCapture();
}

int HolderWindow::OnMouseMove(const POINT &pt)
{
	if (m_resizing)
	{
		RECT clientRect;
		GetClientRect(m_hwnd, &clientRect);

		int newWidth = (std::max)(pt.x + m_resizeDistanceToEdge.value(), 0L);
		SendMessage(GetParent(m_hwnd), WM_USER_HOLDERRESIZED, 0, newWidth);

		return 1;
	}

	if (IsCursorInResizeStartRange(pt))
	{
		SetCursor(m_sizingCursor);
	}

	return 0;
}

bool HolderWindow::OnSetCursor(HWND target)
{
	if (target != m_hwnd)
	{
		return false;
	}

	DWORD cursorPos = GetMessagePos();
	POINT ptCursor = { GET_X_LPARAM(cursorPos), GET_Y_LPARAM(cursorPos) };
	ScreenToClient(m_hwnd, &ptCursor);

	if (IsCursorInResizeStartRange(ptCursor))
	{
		// Without this, the cursor would switch back and forth between the sizing cursor and arrow
		// cursor when near the right edge of the window.
		SetCursor(m_sizingCursor);
		return true;
	}

	return false;
}

bool HolderWindow::IsCursorInResizeStartRange(const POINT &ptCursor)
{
	RECT clientRect;
	GetClientRect(m_hwnd, &clientRect);

	InflateRect(&clientRect,
		-DpiCompatibility::GetInstance().ScaleValue(m_hwnd, RESIZE_START_RANGE), 0);

	return ptCursor.x >= clientRect.right;
}
