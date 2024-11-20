// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Creates and manages a 'display window'. This window
 * can be used to show various information (e.g. file
 * information) and is drawn with a gradient background.
 *
 * Icon:
 * The icon passed in when this class is created will be
 * drawn at 48x48 pixels. To avoid drawing issues, the
 * icon itself should be 48x48 pixels.
 *
 * Text drawing:
 * Text will be drawn in columns. Text height (and width)
 * is calculated, and once a particular column is full,
 * text will be placed in the next column.
 */

#include "stdafx.h"
#include "DisplayWindow.h"
#include "../Helper/Macros.h"
#include "../Helper/WindowSubclassWrapper.h"

DisplayWindow *DisplayWindow::Create(HWND parent, DWInitialSettings_t *initialSettings)
{
	return new DisplayWindow(parent, initialSettings);
}

DisplayWindow::DisplayWindow(HWND parent, DWInitialSettings_t *initialSettings) :
	m_hwnd(CreateDisplayWindow(parent)),
	m_TextColor(initialSettings->TextColor),
	m_CentreColor(initialSettings->CentreColor),
	m_SurroundColor(initialSettings->SurroundColor),
	m_bVertical(FALSE),
	m_hMainIcon(initialSettings->hIcon),
	m_hDisplayFont(initialSettings->hFont)
{
	m_LineSpacing = 20;
	m_LeftIndent = 80;

	m_bSizing = FALSE;
	m_hbmThumbnail = nullptr;
	m_bShowThumbnail = FALSE;
	m_bThumbnailExtracted = FALSE;
	m_bThumbnailExtractionFailed = FALSE;
	m_hBitmapBackground = nullptr;

	InitializeCriticalSection(&m_csDWThumbnails);

	m_hdcBackground = CreateCompatibleDC(GetDC(m_hwnd));

	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(m_hwnd,
		std::bind_front(&DisplayWindow::DisplayWindowProc, this)));
}

DisplayWindow::~DisplayWindow()
{
	DeleteCriticalSection(&m_csDWThumbnails);

	DeleteDC(m_hdcBackground);
	DeleteObject(m_hBitmapBackground);

	DestroyIcon(m_hMainIcon);
}

HWND DisplayWindow::CreateDisplayWindow(HWND parent)
{
	static bool classRegistered = false;

	if (!classRegistered)
	{
		auto res = RegisterDisplayWindowClass();
		CHECK(res);

		classRegistered = true;
	}

	HWND displayWindow =
		CreateWindow(WINDOW_NAME, EMPTY_STRING, WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS, 0, 0, 0, 0,
			parent, nullptr, GetModuleHandle(nullptr), nullptr);
	CHECK(displayWindow);

	return displayWindow;
}

ATOM DisplayWindow::RegisterDisplayWindowClass()
{
	WNDCLASS windowClass = {};
	windowClass.style = 0;
	windowClass.lpfnWndProc = DefWindowProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandle(nullptr);
	windowClass.hIcon = nullptr;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.hbrBackground = nullptr;
	windowClass.lpszMenuName = nullptr;
	windowClass.lpszClassName = CLASS_NAME;
	return RegisterClass(&windowClass);
}

LRESULT DisplayWindow::DisplayWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_LBUTTONUP:
		m_bSizing = FALSE;
		ReleaseCapture();
		break;

	case WM_MOUSEMOVE:
		return OnMouseMove(lParam);

	case WM_LBUTTONDOWN:
		OnLButtonDown(lParam);
		break;

	case WM_RBUTTONUP:
		OnRButtonUp(wParam, lParam);
		break;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc;
		RECT updateRect;
		RECT rc;

		GetUpdateRect(hwnd, &updateRect, FALSE);
		GetClientRect(hwnd, &rc);

		hdc = BeginPaint(hwnd, &ps);

		PatchBackground(hdc, &rc, &updateRect);

		EndPaint(hwnd, &ps);
	}
	break;

	case DWM_BUFFERTEXT:
	{
		LineData_t ld;
		TCHAR *pszText = nullptr;

		pszText = (TCHAR *) lParam;

		if (pszText != nullptr)
		{
			StringCchCopy(ld.szText, std::size(ld.szText), pszText);

			m_LineList.push_back(ld);
		}

		/* TODO: Optimize? */
		RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE);
	}
	break;

	case DWM_CLEARTEXTBUFFER:
	{
		m_LineList.clear();
	}
	break;

	case DWM_SETLINE:
	{
		TCHAR *pszText = nullptr;
		std::vector<LineData_t>::iterator itr;
		unsigned int iLine;
		unsigned int i = 0;

		iLine = (int) wParam;
		pszText = (TCHAR *) lParam;

		if (pszText != nullptr && iLine < m_LineList.size())
		{
			for (itr = m_LineList.begin(); itr != m_LineList.end(); itr++)
			{
				if (i == iLine)
				{
					StringCchCopy(itr->szText, std::size(itr->szText), pszText);
					break;
				}

				i++;
			}
		}

		/* TODO: Optimize? */
		RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE);
	}
	break;

	case DWM_SETTHUMBNAILFILE:
		OnSetThumbnailFile(wParam, lParam);
		RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE);
		break;

	case DWM_GETCENTRECOLOR:
		return m_CentreColor.ToCOLORREF();

	case DWM_GETSURROUNDCOLOR:
		return m_SurroundColor.ToCOLORREF();

	case DWM_SETCENTRECOLOR:
	{
		m_CentreColor.SetFromCOLORREF((COLORREF) wParam);
		HDC hdc;
		hdc = GetDC(hwnd);
		RECT rc;
		GetClientRect(hwnd, &rc);
		DrawGradientFill(hdc, &rc);
		ReleaseDC(hwnd, hdc);
		RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE);
	}
	break;

	case DWM_SETSURROUNDCOLOR:
	{
		m_SurroundColor.SetFromCOLORREF((COLORREF) wParam);
		HDC hdc;
		hdc = GetDC(hwnd);
		RECT rc;
		GetClientRect(hwnd, &rc);
		DrawGradientFill(hdc, &rc);
		ReleaseDC(hwnd, hdc);
		RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE);
	}
	break;

	case DWM_GETFONT:
		HFONT *hFont;
		hFont = (HFONT *) wParam;
		*hFont = m_hDisplayFont;
		break;

	case DWM_SETFONT:
		OnSetFont((HFONT) wParam);
		break;

	case DWM_GETTEXTCOLOR:
		return m_TextColor;

	case DWM_SETTEXTCOLOR:
		OnSetTextColor((COLORREF) wParam);
		break;

	case DWM_DRAWGRADIENTFILL:
		PatchBackground((HDC) wParam, (RECT *) lParam, (RECT *) lParam);
		break;

	case WM_ERASEBKGND:
		HDC hdc;
		hdc = GetDC(hwnd);
		RECT rc;
		RECT updateRect;
		GetUpdateRect(hwnd, &updateRect, FALSE);
		GetClientRect(hwnd, &rc);
		PatchBackground(hdc, &rc, &updateRect);
		ReleaseDC(hwnd, hdc);
		return 1;

	case WM_SIZE:
		OnSize(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_USER_DISPLAYWINDOWMOVED:
		m_bVertical = (BOOL) wParam;
		InvalidateRect(m_hwnd, nullptr, TRUE);
		break;

	case WM_NCDESTROY:
		delete this;
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

HWND DisplayWindow::GetHWND() const
{
	return m_hwnd;
}
