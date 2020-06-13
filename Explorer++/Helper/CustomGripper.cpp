// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "CustomGripper.h"
#include "DpiCompatibility.h"
#include <Uxtheme.h>
#include <VSStyle.h>

// clang-format off
// wil::unique_htheme is only defined if Uxtheme.h is included before wil/resource.h.
#include <wil/resource.h>
// clang-format on

namespace
{
	constexpr int PART_ID = SBP_SIZEBOX;
	constexpr int STATE_ID = SZB_RIGHTALIGN;

	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnPaint(HWND hwnd);

	DpiCompatibility g_dpiCompat;
	bool g_isAppThemed;
	wil::unique_htheme g_theme;
	SIZE g_size;
}

void CustomGripper::Initialize(HWND mainWindow, COLORREF backgroundColor)
{
	WNDCLASS wc;
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hIcon = nullptr;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(backgroundColor);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = CLASS_NAME;
	const ATOM res = RegisterClass(&wc);
	assert(res);

	g_isAppThemed = IsAppThemed();

	if (g_isAppThemed)
	{
		g_theme.reset(OpenThemeData(mainWindow, L"SCROLLBAR"));

		wil::unique_hdc screenDC(GetDC(nullptr));
		GetThemePartSize(
			g_theme.get(), screenDC.get(), PART_ID, STATE_ID, nullptr, TS_DRAW, &g_size);
	}
	else
	{
		g_size = { 16, 16 };
	}
}

namespace
{
	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_PAINT:
			OnPaint(hwnd);
			return 0;
		}

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	void OnPaint(HWND hwnd)
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		RECT rect;
		GetClientRect(hwnd, &rect);

		// Visual styles can only be turned off prior to Windows 8. So drawing without visual styles
		// is something that's only required to support Windows 7.
		if (g_isAppThemed)
		{
			if (IsThemeBackgroundPartiallyTransparent(g_theme.get(), PART_ID, STATE_ID))
			{
				DrawThemeParentBackground(hwnd, hdc, &ps.rcPaint);
			}

			DrawThemeBackground(g_theme.get(), hdc, PART_ID, STATE_ID, &rect, &ps.rcPaint);
		}
		else
		{
			DrawFrameControl(hdc, &rect, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
		}

		EndPaint(hwnd, &ps);
	}
}

SIZE CustomGripper::GetDpiScaledSize(HWND parentWindow)
{
	if (g_isAppThemed)
	{
		return g_size;
	}
	else
	{
		UINT dpi = g_dpiCompat.GetDpiForWindow(parentWindow);
		return { MulDiv(g_size.cx, dpi, USER_DEFAULT_SCREEN_DPI),
			MulDiv(g_size.cy, dpi, USER_DEFAULT_SCREEN_DPI) };
	}
}