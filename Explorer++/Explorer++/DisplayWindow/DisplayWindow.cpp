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

namespace
{
const TCHAR CLASS_NAME[] = _T("DisplayWindow");
const TCHAR WINDOW_NAME[] = _T("DisplayWindow");
}

ULONG_PTR token;

namespace
{
BOOL RegisterDisplayWindowClass()
{
	Gdiplus::GdiplusStartupInput startupInput;

	WNDCLASS wc;
	wc.style = 0;
	wc.lpfnWndProc = DisplayWindow::DisplayWindowProcStub;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = sizeof(DisplayWindow *);
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hIcon = nullptr;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = CLASS_NAME;

	if (!RegisterClass(&wc))
	{
		return FALSE;
	}

	Gdiplus::GdiplusStartup(&token, &startupInput, nullptr);

	return TRUE;
}
}

HWND CreateDisplayWindow(HWND parent, DWInitialSettings_t *pSettings)
{
	RegisterDisplayWindowClass();

	HWND hDisplayWindow =
		CreateWindow(WINDOW_NAME, EMPTY_STRING, WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS, 0, 0, 0, 0,
			parent, nullptr, GetModuleHandle(nullptr), reinterpret_cast<LPVOID>(pSettings));

	return hDisplayWindow;
}

DisplayWindow::DisplayWindow(HWND hDisplayWindow, DWInitialSettings_t *pInitialSettings) :
	m_hDisplayWindow(hDisplayWindow),
	m_TextColor(pInitialSettings->TextColor),
	m_CentreColor(pInitialSettings->CentreColor),
	m_SurroundColor(pInitialSettings->SurroundColor),
	m_bVertical(FALSE),
	m_hMainIcon(pInitialSettings->hIcon),
	m_hDisplayFont(pInitialSettings->hFont)
{
	g_ObjectCount++;

	m_LineSpacing = 20;
	m_LeftIndent = 80;

	m_bSizing = FALSE;
	m_hbmThumbnail = nullptr;
	m_bShowThumbnail = FALSE;
	m_bThumbnailExtracted = FALSE;
	m_bThumbnailExtractionFailed = FALSE;
	m_hBitmapBackground = nullptr;

	InitializeCriticalSection(&m_csDWThumbnails);
}

DisplayWindow::~DisplayWindow()
{
	DeleteCriticalSection(&m_csDWThumbnails);

	DeleteDC(m_hdcBackground);
	DeleteObject(m_hBitmapBackground);

	DestroyIcon(m_hMainIcon);

	g_ObjectCount--;

	if (g_ObjectCount == 0)
	{
		Gdiplus::GdiplusShutdown(token);
	}
}

LRESULT CALLBACK DisplayWindow::DisplayWindowProcStub(HWND hwnd, UINT msg, WPARAM wParam,
	LPARAM lParam)
{
	auto *pdw = reinterpret_cast<DisplayWindow *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	switch (msg)
	{
	case WM_CREATE:
	{
		auto *pSettings = reinterpret_cast<DWInitialSettings_t *>(
			reinterpret_cast<CREATESTRUCT *>(lParam)->lpCreateParams);

		pdw = new DisplayWindow(hwnd, pSettings);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pdw));
	}
	break;

	case WM_NCDESTROY:
		delete pdw;
		return 0;
	}

	return pdw->DisplayWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK DisplayWindow::DisplayWindowProc(HWND displayWindow, UINT msg, WPARAM wParam,
	LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
		m_hdcBackground = CreateCompatibleDC(GetDC(displayWindow));
		break;

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

		GetUpdateRect(displayWindow, &updateRect, FALSE);
		GetClientRect(displayWindow, &rc);

		hdc = BeginPaint(displayWindow, &ps);

		PatchBackground(hdc, &rc, &updateRect);

		EndPaint(displayWindow, &ps);
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
		RedrawWindow(displayWindow, nullptr, nullptr, RDW_INVALIDATE);
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
		RedrawWindow(displayWindow, nullptr, nullptr, RDW_INVALIDATE);
	}
	break;

	case DWM_SETTHUMBNAILFILE:
		OnSetThumbnailFile(wParam, lParam);
		RedrawWindow(displayWindow, nullptr, nullptr, RDW_INVALIDATE);
		break;

	case DWM_GETCENTRECOLOR:
		return m_CentreColor.ToCOLORREF();

	case DWM_GETSURROUNDCOLOR:
		return m_SurroundColor.ToCOLORREF();

	case DWM_SETCENTRECOLOR:
	{
		m_CentreColor.SetFromCOLORREF((COLORREF) wParam);
		HDC hdc;
		hdc = GetDC(displayWindow);
		RECT rc;
		GetClientRect(displayWindow, &rc);
		DrawGradientFill(hdc, &rc);
		ReleaseDC(displayWindow, hdc);
		RedrawWindow(displayWindow, nullptr, nullptr, RDW_INVALIDATE);
	}
	break;

	case DWM_SETSURROUNDCOLOR:
	{
		m_SurroundColor.SetFromCOLORREF((COLORREF) wParam);
		HDC hdc;
		hdc = GetDC(displayWindow);
		RECT rc;
		GetClientRect(displayWindow, &rc);
		DrawGradientFill(hdc, &rc);
		ReleaseDC(displayWindow, hdc);
		RedrawWindow(displayWindow, nullptr, nullptr, RDW_INVALIDATE);
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
		hdc = GetDC(displayWindow);
		RECT rc;
		RECT updateRect;
		GetUpdateRect(displayWindow, &updateRect, FALSE);
		GetClientRect(displayWindow, &rc);
		PatchBackground(hdc, &rc, &updateRect);
		ReleaseDC(displayWindow, hdc);
		return 1;

	case WM_SIZE:
		OnSize(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_USER_DISPLAYWINDOWMOVED:
		m_bVertical = (BOOL) wParam;
		InvalidateRect(m_hDisplayWindow, nullptr, TRUE);
		break;
	}

	return DefWindowProc(displayWindow, msg, wParam, lParam);
}
