// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/WindowSubclassWrapper.h"
#include <uxtheme.h>
#include <memory>
#include <vector>

// clang-format off
#include <wil/resource.h>
// clang-format on

// Custom draws a standard group box (by handling the WM_PAINT) message. The output is very close to
// that of the regular control, though there are some minor differences (e.g. slight differences in
// spacing between the group box caption and its border).
class DarkModeGroupBox
{
public:
	DarkModeGroupBox(HWND groupBox);

private:
	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnPaint(HWND hwnd);

	wil::unique_htheme m_theme;
	std::unique_ptr<WindowSubclassWrapper> m_windowSubclass;
};