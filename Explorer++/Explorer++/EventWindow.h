// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/SignalWrapper.h"
#include <wil/resource.h>
#include <memory>

class WindowSubclass;

// Allows callers to observe messages that are broadcast to top-level windows (e.g.
// WM_DEVICECHANGE).
class EventWindow
{
public:
	EventWindow();

	// Signals
	SignalWrapper<EventWindow, void(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)>
		windowMessageSignal;

private:
	static constexpr wchar_t CLASS_NAME[] = L"Explorer++EventWindowClass";

	static ATOM RegisterWindowClass();
	LRESULT Subclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	wil::unique_hwnd m_hwnd;
	std::unique_ptr<WindowSubclass> m_subclass;
};
