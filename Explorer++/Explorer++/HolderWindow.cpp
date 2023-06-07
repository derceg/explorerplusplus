// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Manages the 'holder window'. This window acts as a generic
 * container for child windows.
 */

#include "stdafx.h"
#include "HolderWindow.h"
#include "CoreInterface.h"
#include "DarkModeHelper.h"
#include "ToolbarHelper.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/WindowHelper.h"

#define FOLDERS_TEXT_X 5
#define FOLDERS_TEXT_Y 2

HolderWindow *HolderWindow::Create(HWND parent, const std::wstring &caption, DWORD style,
	const std::wstring &closeButtonTooltip, CoreInterface *coreInterface)
{
	return new HolderWindow(parent, caption, style, closeButtonTooltip, coreInterface);
}

HolderWindow::HolderWindow(HWND parent, const std::wstring &caption, DWORD style,
	const std::wstring &closeButtonTooltip, CoreInterface *coreInterface) :
	m_hwnd(CreateHolderWindow(parent, caption, style)),
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

	std::tie(m_toolbar, m_toolbarImageList) = ToolbarHelper::CreateCloseButtonToolbar(m_hwnd,
		CLOSE_BUTTON_ID, closeButtonTooltip, coreInterface->GetIconResourceLoader());

	SIZE toolbarSize;
	[[maybe_unused]] auto sizeRes =
		SendMessage(m_toolbar, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&toolbarSize));
	assert(sizeRes);
	SetWindowPos(m_toolbar, nullptr, 0, 0, toolbarSize.cx, toolbarSize.cy,
		SWP_NOZORDER | SWP_NOMOVE);
}

HWND HolderWindow::CreateHolderWindow(HWND parent, const std::wstring &caption, DWORD style)
{
	RegisterHolderWindowClass();

	return CreateWindowEx(WS_EX_CONTROLPARENT, CLASS_NAME, caption.c_str(), style, 0, 0, 0, 0,
		parent, nullptr, GetModuleHandle(nullptr), this);
}

ATOM HolderWindow::RegisterHolderWindowClass()
{
	WNDCLASS wc;
	wc.style = 0;
	wc.lpfnWndProc = WndProcStub;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = sizeof(HolderWindow *);
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hIcon = nullptr;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = CLASS_NAME;

	return RegisterClass(&wc);
}

LRESULT CALLBACK HolderWindow::WndProcStub(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	auto *holderWindow = reinterpret_cast<HolderWindow *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	switch (msg)
	{
	case WM_CREATE:
	{
		auto *createInfo = reinterpret_cast<CREATESTRUCT *>(lParam);
		holderWindow = reinterpret_cast<HolderWindow *>(createInfo->lpCreateParams);

		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(holderWindow));
	}
	break;

	case WM_NCDESTROY:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
		delete holderWindow;
		return 0;
	}

	if (holderWindow)
	{
		return holderWindow->WndProc(hwnd, msg, wParam, lParam);
	}
	else
	{
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

LRESULT CALLBACK HolderWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

	case WM_COMMAND:
		if (HIWORD(wParam) == 0)
		{
			switch (LOWORD(wParam))
			{
			case CLOSE_BUTTON_ID:
				if (m_closeButtonClickedCallback)
				{
					m_closeButtonClickedCallback();
				}
				return 0;
			}
		}
		break;

	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT:
		OnPaint();
		return 0;

	// The toolbar that's contained within this window has a transparent background, with the
	// background from the parent being drawn underneath it. For that background to be correctly
	// drawn, WM_PRINTCLIENT needs to be handled.
	case WM_PRINTCLIENT:
		OnPrintClient(reinterpret_cast<HDC>(wParam));
		return 0;

	case WM_SIZE:
		OnSize(LOWORD(lParam), HIWORD(lParam));
		break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void HolderWindow::OnPaint()
{
	PAINTSTRUCT ps;
	BeginPaint(m_hwnd, &ps);
	PerformPaint(ps);
	EndPaint(m_hwnd, &ps);
}

void HolderWindow::OnPrintClient(HDC hdc)
{
	PAINTSTRUCT ps = {};
	ps.hdc = hdc;
	GetClientRect(m_hwnd, &ps.rcPaint);
	PerformPaint(ps);
}

void HolderWindow::PerformPaint(const PAINTSTRUCT &ps)
{
	auto &darkModeHelper = DarkModeHelper::GetInstance();
	HBRUSH backgroundBrush;

	if (darkModeHelper.IsDarkModeEnabled())
	{
		backgroundBrush = darkModeHelper.GetBackgroundBrush();
	}
	else
	{
		backgroundBrush = GetSysColorBrush(COLOR_BTNFACE);
	}

	FillRect(ps.hdc, &ps.rcPaint, backgroundBrush);

	std::wstring caption = GetWindowString(m_hwnd);
	auto selectFont = wil::SelectObject(ps.hdc, m_font.get());
	SetBkMode(ps.hdc, TRANSPARENT);

	if (darkModeHelper.IsDarkModeEnabled())
	{
		SetTextColor(ps.hdc, DarkModeHelper::TEXT_COLOR);
	}

	TextOut(ps.hdc, FOLDERS_TEXT_X, FOLDERS_TEXT_Y, caption.c_str(),
		static_cast<int>(caption.size()));
}

void HolderWindow::OnSize(int width, int height)
{
	UNREFERENCED_PARAMETER(height);

	auto &dpiCompatibility = DpiCompatibility::GetInstance();
	int scaledCloseToolbarXOffset =
		dpiCompatibility.ScaleValue(m_toolbar, ToolbarHelper::CLOSE_TOOLBAR_X_OFFSET);
	int scaledCloseToolbarYOffset =
		dpiCompatibility.ScaleValue(m_toolbar, ToolbarHelper::CLOSE_TOOLBAR_Y_OFFSET);

	RECT toolbarRect;
	GetClientRect(m_toolbar, &toolbarRect);
	SetWindowPos(m_toolbar, nullptr, width - GetRectWidth(&toolbarRect) - scaledCloseToolbarXOffset,
		scaledCloseToolbarYOffset, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
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

		if (m_resizedCallback)
		{
			m_resizedCallback(newWidth);
		}

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

HWND HolderWindow::GetHWND() const
{
	return m_hwnd;
}

void HolderWindow::SetResizedCallback(ResizedCallback callback)
{
	m_resizedCallback = callback;
}

void HolderWindow::SetCloseButtonClickedCallback(CloseButtonClickedCallback callback)
{
	m_closeButtonClickedCallback = callback;
}
