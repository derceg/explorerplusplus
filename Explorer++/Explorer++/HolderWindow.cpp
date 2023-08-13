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
#include "MainFontSetter.h"
#include "SystemFontHelper.h"
#include "ToolbarHelper.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/WindowHelper.h"

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
	LOGFONT systemFont = GetDefaultSystemFont(m_hwnd);
	m_defaultFont.reset(CreateFontIndirect(&systemFont));
	assert(m_defaultFont);

	m_font = m_defaultFont.get();

	std::tie(m_toolbar, m_toolbarImageList) = ToolbarHelper::CreateCloseButtonToolbar(m_hwnd,
		CLOSE_BUTTON_ID, closeButtonTooltip, coreInterface->GetIconResourceLoader());

	SIZE toolbarSize;
	[[maybe_unused]] auto sizeRes =
		SendMessage(m_toolbar, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&toolbarSize));
	assert(sizeRes);
	SetWindowPos(m_toolbar, nullptr, 0, 0, toolbarSize.cx, toolbarSize.cy,
		SWP_NOZORDER | SWP_NOMOVE);

	m_tooltipFontSetter = std::make_unique<MainFontSetter>(
		reinterpret_cast<HWND>(SendMessage(m_toolbar, TB_GETTOOLTIPS, 0, 0)),
		coreInterface->GetConfig());

	m_fontSetter = std::make_unique<MainFontSetter>(m_hwnd, coreInterface->GetConfig());

	m_initialized = true;
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

	case WM_SETFONT:
		OnSetFont(reinterpret_cast<HFONT>(wParam), lParam);
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

void HolderWindow::OnSetFont(HFONT font, bool redraw)
{
	SetFont(font);

	if (redraw)
	{
		RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
	}
}

void HolderWindow::SetFont(HFONT font)
{
	if (font)
	{
		m_font = font;
	}
	else
	{
		m_font = m_defaultFont.get();
	}

	m_captionSectionHeight.reset();
	UpdateLayout();
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
	auto selectFont = wil::SelectObject(ps.hdc, m_font);
	SetBkMode(ps.hdc, TRANSPARENT);

	if (darkModeHelper.IsDarkModeEnabled())
	{
		SetTextColor(ps.hdc, DarkModeHelper::TEXT_COLOR);
	}

	RECT toolbarRect;
	GetWindowRect(m_toolbar, &toolbarRect);
	MapWindowPoints(HWND_DESKTOP, m_hwnd, reinterpret_cast<LPPOINT>(&toolbarRect), 2);

	RECT textRect = { CAPTION_SECTION_HORIZONTAL_PADDING, 0, toolbarRect.left,
		GetCaptionSectionHeight() };
	DrawText(ps.hdc, caption.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

void HolderWindow::OnSize(int width, int height)
{
	UpdateLayout(width, height);
}

void HolderWindow::UpdateLayout()
{
	RECT rc;
	GetClientRect(m_hwnd, &rc);
	UpdateLayout(GetRectWidth(&rc), GetRectHeight(&rc));
}

void HolderWindow::UpdateLayout(int width, int height)
{
	if (!m_initialized)
	{
		return;
	}

	auto deferInfo = BeginDeferWindowPos(2);

	RECT toolbarRect;
	GetClientRect(m_toolbar, &toolbarRect);

	auto &dpiCompatibility = DpiCompatibility::GetInstance();
	int captionHorizontalPadding =
		dpiCompatibility.ScaleValue(m_toolbar, CAPTION_SECTION_HORIZONTAL_PADDING);
	int captionSectionHeight = GetCaptionSectionHeight();

	deferInfo = DeferWindowPos(deferInfo, m_toolbar, nullptr,
		width - GetRectWidth(&toolbarRect) - captionHorizontalPadding,
		(captionSectionHeight - GetRectHeight(&toolbarRect)) / 2, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

	if (m_contentChild)
	{
		auto contentHorizontalPadding =
			dpiCompatibility.ScaleValue(m_hwnd, CONTENT_SECTION_RIGHT_PADDING);
		deferInfo = DeferWindowPos(deferInfo, m_contentChild, nullptr, 0, captionSectionHeight,
			width - contentHorizontalPadding, height - captionSectionHeight, SWP_NOZORDER);
	}

	[[maybe_unused]] auto res = EndDeferWindowPos(deferInfo);
	assert(res);
}

int HolderWindow::GetCaptionSectionHeight()
{
	if (!m_captionSectionHeight)
	{
		m_captionSectionHeight = CalculateCaptionSectionHeight();
	}

	return *m_captionSectionHeight;
}

int HolderWindow::CalculateCaptionSectionHeight()
{
	RECT rc;
	[[maybe_unused]] auto res = GetClientRect(m_toolbar, &rc);
	assert(res);

	SIZE textSize;
	auto hdc = wil::GetDC(m_hwnd);
	auto selectFont = wil::SelectObject(hdc.get(), m_font);
	auto text = GetWindowString(m_hwnd);
	[[maybe_unused]] auto textExtentRes =
		GetTextExtentPoint32(hdc.get(), text.c_str(), static_cast<int>(text.length()), &textSize);
	assert(textExtentRes);

	auto verticalPadding =
		DpiCompatibility::GetInstance().ScaleValue(m_hwnd, CAPTION_SECTION_VERTICAL_PADDING);
	return (std::max)(textSize.cy, rc.bottom) + (verticalPadding * 2);
}

void HolderWindow::OnLButtonDown(const POINT &pt)
{
	if (IsCursorInResizeStartRange(pt))
	{
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

void HolderWindow::SetContentChild(HWND contentChild)
{
	assert(GetParent(contentChild) == m_hwnd);
	m_contentChild = contentChild;
}

void HolderWindow::SetResizedCallback(ResizedCallback callback)
{
	m_resizedCallback = callback;
}

void HolderWindow::SetCloseButtonClickedCallback(CloseButtonClickedCallback callback)
{
	m_closeButtonClickedCallback = callback;
}
