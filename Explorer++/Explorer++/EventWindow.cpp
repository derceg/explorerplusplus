// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "EventWindow.h"
#include "../Helper/WindowSubclass.h"

EventWindow::EventWindow()
{
	static bool classRegistered = false;

	if (!classRegistered)
	{
		auto res = RegisterWindowClass();
		CHECK_NE(res, 0);

		classRegistered = true;
	}

	m_hwnd.reset(CreateWindow(CLASS_NAME, L"", WS_DISABLED, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, GetModuleHandle(nullptr), nullptr));
	CHECK(m_hwnd);

	m_subclass = std::make_unique<WindowSubclass>(m_hwnd.get(),
		std::bind_front(&EventWindow::Subclass, this));
}

ATOM EventWindow::RegisterWindowClass()
{
	WNDCLASS windowClass = {};
	windowClass.lpfnWndProc = DefWindowProc;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.lpszClassName = CLASS_NAME;
	windowClass.hInstance = GetModuleHandle(nullptr);
	windowClass.style = 0;
	return RegisterClass(&windowClass);
}

LRESULT EventWindow::Subclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	windowMessageSignal.m_signal(hwnd, msg, wParam, lParam);
	return DefSubclassProc(hwnd, msg, wParam, lParam);
}
