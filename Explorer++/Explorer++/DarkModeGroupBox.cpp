// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DarkModeGroupBox.h"
#include "DarkModeHelper.h"
#include "../Helper/WindowHelper.h"
#include <VSStyle.h>

DarkModeGroupBox::DarkModeGroupBox(HWND groupBox) : m_theme(OpenThemeData(nullptr, L"BUTTON"))
{
	m_windowSubclass = std::make_unique<WindowSubclassWrapper>(groupBox,
		std::bind(&DarkModeGroupBox::WndProc, this, std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4),
		0);
}

LRESULT CALLBACK DarkModeGroupBox::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_PAINT:
		OnPaint(hwnd);
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void DarkModeGroupBox::OnPaint(HWND hwnd)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);

	RECT rect;
	GetClientRect(hwnd, &rect);

	std::wstring text = GetWindowString(hwnd);
	assert(!text.empty());

	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, DarkModeHelper::FOREGROUND_COLOR);

	NONCLIENTMETRICS metrics;
	metrics.cbSize = sizeof(metrics);
	m_dpiCompat.SystemParametersInfoForDpi(
		SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0, m_dpiCompat.GetDpiForWindow(hwnd));

	wil::unique_hfont captionFont(CreateFontIndirect(&metrics.lfCaptionFont));
	wil::unique_select_object object(SelectObject(hdc, captionFont.get()));

	RECT textRect = rect;
	DrawText(hdc, text.c_str(), static_cast<int>(text.size()), &textRect, DT_CALCRECT);

	// The top border of the group box isn't anchored to the top of the control. Rather, it cuts
	// midway through the caption text. That is, the text is anchored to the top of the control and
	// the border starts at the vertical mid-point of the text.
	RECT groupBoxRect = rect;
	groupBoxRect.top += GetRectHeight(&textRect) / 2;

	// The group box border isn't shown behind the caption; instead, the text appears as if its
	// drawn directly on top of the parent.
	DrawThemeBackground(m_theme.get(), hdc, BP_GROUPBOX, GBS_NORMAL, &groupBoxRect, &ps.rcPaint);

	// It appears this offset isn't DPI-adjusted.
	OffsetRect(&textRect, 9, 0);

	// As with the above, it appears this offset isn't DPI-adjusted.
	RECT textBackgroundRect = textRect;
	textBackgroundRect.left -= 2;
	DrawThemeParentBackground(hwnd, hdc, &textBackgroundRect);

	DrawText(hdc, text.c_str(), static_cast<int>(text.size()), &textRect, DT_LEFT);

	EndPaint(hwnd, &ps);
}