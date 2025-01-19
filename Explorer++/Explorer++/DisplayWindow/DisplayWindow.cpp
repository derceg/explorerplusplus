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
#include "Config.h"
#include "MainResource.h"
#include "../Helper/WindowSubclass.h"

DisplayWindow *DisplayWindow::Create(HWND parent, const Config *config)
{
	return new DisplayWindow(parent, config);
}

DisplayWindow::DisplayWindow(HWND parent, const Config *config) :
	m_hwnd(CreateDisplayWindow(parent)),
	m_config(config),
	m_mainIcon(static_cast<HICON>(LoadImage(GetModuleHandle(nullptr),
		MAKEINTRESOURCE(IDI_DISPLAYWINDOW), IMAGE_ICON, 0, 0, LR_CREATEDIBSECTION))),
	m_font(CreateFontIndirect(&config->displayWindowFont.get())),
	m_bVertical(FALSE)
{
	m_LineSpacing = 20;
	m_LeftIndent = 80;

	m_bSizing = FALSE;
	m_hbmThumbnail = nullptr;
	m_bShowThumbnail = FALSE;
	m_bThumbnailExtracted = FALSE;
	m_bThumbnailExtractionFailed = FALSE;

	InitializeCriticalSection(&m_csDWThumbnails);

	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(m_hwnd,
		std::bind_front(&DisplayWindow::DisplayWindowProc, this)));

	m_connections.push_back(m_config->displayWindowCentreColor.addObserver(
		std::bind(&DisplayWindow::OnDisplayConfigChanged, this)));
	m_connections.push_back(m_config->displayWindowSurroundColor.addObserver(
		std::bind(&DisplayWindow::OnDisplayConfigChanged, this)));
	m_connections.push_back(m_config->displayWindowFont.addObserver(
		std::bind(&DisplayWindow::OnFontConfigChanged, this)));
	m_connections.push_back(m_config->displayWindowTextColor.addObserver(
		std::bind(&DisplayWindow::OnDisplayConfigChanged, this)));
}

DisplayWindow::~DisplayWindow()
{
	DeleteCriticalSection(&m_csDWThumbnails);
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

	HWND displayWindow = CreateWindow(WINDOW_NAME, L"", WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS, 0,
		0, 0, 0, parent, nullptr, GetModuleHandle(nullptr), nullptr);
	CHECK(displayWindow);

	return displayWindow;
}

ATOM DisplayWindow::RegisterDisplayWindowClass()
{
	WNDCLASS windowClass = {};
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
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

		Draw(hdc, &rc, &updateRect);

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

void DisplayWindow::OnDisplayConfigChanged()
{
	RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE);
}

void DisplayWindow::OnFontConfigChanged()
{
	m_font.reset(CreateFontIndirect(&m_config->displayWindowFont.get()));

	RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE);
}
