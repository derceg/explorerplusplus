// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "WinRTBaseWrapper.h"
#include <functional>

using GetWindowCallback = std::function<HWND()>;

class OleWindow : public winrt::implements<OleWindow, IOleWindow, winrt::non_agile>
{
public:
	OleWindow(GetWindowCallback getWindowCallback);

	// IOleWindow
	IFACEMETHODIMP GetWindow(HWND *outputHwnd);
	IFACEMETHODIMP ContextSensitiveHelp(BOOL enterMode);

private:
	GetWindowCallback m_getWindowCallback;
};
