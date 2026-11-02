// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ViewBase.h"
#include <wil/resource.h>

class BrowserWindowFakeView : public ViewBase
{
public:
	BrowserWindowFakeView(const RECT &bounds);

private:
	static constexpr wchar_t CLASS_NAME[] = L"TestExplorer++BrowserWindowClass";

	static HWND CreateBrowserWindow(const RECT &bounds);
	static void RegisterBrowserWindowClass();

	wil::unique_hwnd m_ownedWindow;
};
