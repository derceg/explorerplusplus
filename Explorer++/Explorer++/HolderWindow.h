// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Literals.h"
#include <wil/resource.h>
#include <optional>

static constexpr int WM_APP_HOLDER_RESIZED = WM_APP + 300;

class HolderWindow
{
public:
	static HolderWindow *Create(HWND parent, const std::wstring &caption, DWORD style);

	HWND GetHWND() const;

private:
	static constexpr WCHAR CLASS_NAME[] = L"Holder";

	// A resize can be started if the cursor is this many pixels from the right edge of the window.
	static constexpr int RESIZE_START_RANGE = 6_px;

	HolderWindow(HWND parent, const std::wstring &caption, DWORD style);
	HWND CreateHolderWindow(HWND parent, const std::wstring &caption, DWORD style);
	static ATOM RegisterHolderWindowClass();

	static LRESULT CALLBACK WndProcStub(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void OnEraseBackground(HDC hdc);
	void OnPaint(HWND hwnd);
	void OnLButtonDown(const POINT &pt);
	void OnLButtonUp();
	int OnMouseMove(const POINT &pt);
	bool OnSetCursor(HWND target);
	bool IsCursorInResizeStartRange(const POINT &ptCursor);

	const HWND m_hwnd;
	wil::unique_hfont m_font;
	HCURSOR m_sizingCursor;
	bool m_resizing = false;
	std::optional<int> m_resizeDistanceToEdge;
};
