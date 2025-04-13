// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MessageWindowHelper.h"

namespace
{

ATOM RegisterMessageWindowClass()
{
	WNDCLASS windowClass = {};
	windowClass.lpfnWndProc = DefWindowProc;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.lpszClassName = MessageWindowHelper::MESSAGE_CLASS_NAME;
	windowClass.hInstance = GetModuleHandle(nullptr);
	windowClass.style = 0;
	return RegisterClass(&windowClass);
}

}

namespace MessageWindowHelper
{

wil::unique_hwnd CreateMessageOnlyWindow(const std::wstring &windowName)
{
	static bool messageWindowClassRegistered = false;

	if (!messageWindowClassRegistered)
	{
		auto res = RegisterMessageWindowClass();
		CHECK_NE(res, 0);

		messageWindowClassRegistered = true;
	}

	HWND hwnd = CreateWindow(MESSAGE_CLASS_NAME, windowName.c_str(), WS_DISABLED, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, nullptr,
		GetModuleHandle(nullptr), nullptr);
	CHECK(hwnd);

	return wil::unique_hwnd(hwnd);
}

}
