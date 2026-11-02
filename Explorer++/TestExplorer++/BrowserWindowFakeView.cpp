// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "BrowserWindowFakeView.h"

BrowserWindowFakeView::BrowserWindowFakeView(const RECT &bounds) :
	ViewBase(CreateBrowserWindow(bounds)),
	m_ownedWindow(GetHWND())
{
}

HWND BrowserWindowFakeView::CreateBrowserWindow(const RECT &bounds)
{
	static bool classRegistered = false;

	if (!classRegistered)
	{
		RegisterBrowserWindowClass();

		classRegistered = true;
	}

	HWND hwnd = CreateWindow(CLASS_NAME, L"", WS_OVERLAPPEDWINDOW | WS_DISABLED, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, GetModuleHandle(nullptr),
		nullptr);
	CHECK(hwnd);

	WINDOWPLACEMENT placement = {};
	placement.length = sizeof(placement);
	BOOL res = GetWindowPlacement(hwnd, &placement);
	CHECK(res);

	placement.showCmd = SW_HIDE;
	placement.rcNormalPosition = bounds;
	res = SetWindowPlacement(hwnd, &placement);
	CHECK(res);

	return hwnd;
}

void BrowserWindowFakeView::RegisterBrowserWindowClass()
{
	WNDCLASS windowClass = {};
	windowClass.lpfnWndProc = DefWindowProc;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.lpszClassName = CLASS_NAME;
	windowClass.hInstance = GetModuleHandle(nullptr);
	windowClass.style = 0;
	auto atom = RegisterClass(&windowClass);
	CHECK(atom);
}
