// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "StatusBarView.h"
#include "../Helper/Controls.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/WindowSubclass.h"

StatusBarView *StatusBarView::Create(HWND parent, const Config *config)
{
	return new StatusBarView(parent, config);
}

StatusBarView::StatusBarView(HWND parent, const Config *config) :
	m_hwnd(CreateStatusBar(parent,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SBARS_SIZEGRIP | WS_CLIPCHILDREN)),
	m_fontSetter(m_hwnd, config),
	m_parts({ 100 })
{
	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(m_hwnd,
		std::bind_front(&StatusBarView::StatusBarSubclass, this)));

	m_fontSetter.fontUpdatedSignal.AddObserver(
		std::bind_front(&StatusBarView::UpdateMinHeight, this));

	UpdateControlParts();

	// Even if the status bar uses the default font, the height won't necessarily be correct. As
	// detailed below, if the text size is increased in Windows, the height of the status bar won't
	// change at 96 DPI. Therefore, setting the minimum height here ensures that the status bar is
	// sized correctly in that situation.
	UpdateMinHeight();
}

void StatusBarView::UpdateMinHeight()
{
	auto hdc = wil::GetDC(m_hwnd);

	auto font = reinterpret_cast<HFONT>(SendMessage(m_hwnd, WM_GETFONT, 0, 0));
	wil::unique_select_object selectFont;

	if (font)
	{
		selectFont = wil::SelectObject(hdc.get(), font);
	}

	wil::unique_htheme theme(OpenThemeData(m_hwnd, L"Status"));
	DCHECK(theme);

	TEXTMETRIC textMetrics;
	HRESULT hr = GetThemeTextMetrics(theme.get(), hdc.get(), 0, 0, &textMetrics);
	DCHECK(SUCCEEDED(hr));

	// From looking into precisely what the status bar control does when it receives a WM_SETFONT
	// message, it appears that it does recalculate its height. However, for whatever reason, the
	// font that's passed to WM_SETFONT will only be used as part of the calculation if the DPI is
	// greater than 96. Otherwise, the default DC font will be used for the calculation.
	//
	// That means that, confusingly, if the DPI is greater than 96, WM_SETFONT will work as expected
	// - the font will be set and the control size will be correctly updated. However, if the DPI is
	// 96, the font will be set, but the control size won't change.
	//
	// The same issue can also be seen when increasing the text size in Windows. Doing that will
	// cause the status bar to use the larger font, but the height of the status bar won't change,
	// leading to the text being potentially cut off.
	//
	// Therefore, to work around those sorts of issues, the minimum height is set here. Note that
	// the height set here will be ignored if it's smaller than the text height calculated by the
	// control. That means that, at 96 DPI, the height will be ignored if the font that's set is
	// smaller than the default font.
	//
	// That shouldn't really be a problem, though, since the control will simply be a bit larger
	// than necessary.
	SendMessage(m_hwnd, SB_SETMINHEIGHT, textMetrics.tmHeight, 0);

	// As per the documentation for SB_SETMINHEIGHT, this will redraw the window.
	SendMessage(m_hwnd, WM_SIZE, 0, 0);
}

HWND StatusBarView::GetHWND() const
{
	return m_hwnd;
}

void StatusBarView::SetParts(const std::vector<int> &parts)
{
	CHECK(VerifyParts(parts));

	m_parts = parts;
	UpdateControlParts();
}

void StatusBarView::SetPartText(int partIndex, const std::wstring &text)
{
	auto res = SendMessage(m_hwnd, SB_SETTEXT, partIndex, reinterpret_cast<LPARAM>(text.c_str()));
	DCHECK(res);
}

LRESULT StatusBarView::StatusBarSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SIZE:
		UpdateControlParts();
		break;

	case WM_NCDESTROY:
		OnNcDestroy();
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

bool StatusBarView::VerifyParts(const std::vector<int> &parts)
{
	if (parts.empty())
	{
		return false;
	}

	int previousPart = 0;

	for (int part : parts)
	{
		if (part < 1 || part > 100)
		{
			return false;
		}

		if (part <= previousPart)
		{
			return false;
		}

		previousPart = part;
	}

	return true;
}

void StatusBarView::UpdateControlParts()
{
	RECT clientRect;
	BOOL res = GetClientRect(m_hwnd, &clientRect);
	CHECK(res);

	int width = GetRectWidth(&clientRect);

	std::vector<int> partWidths;

	for (int part : m_parts)
	{
		if (part == 100)
		{
			partWidths.push_back(-1);
		}
		else
		{
			partWidths.push_back(static_cast<int>((part / 100.0) * width));
		}
	}

	auto setPartsRes = SendMessage(m_hwnd, SB_SETPARTS, m_parts.size(),
		reinterpret_cast<LPARAM>(partWidths.data()));
	DCHECK(setPartsRes);
}

void StatusBarView::OnNcDestroy()
{
	windowDestroyedSignal.m_signal();

	delete this;
}
