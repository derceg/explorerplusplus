// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TestParentView.h"

TestParentView::TestParentView() : ViewBase(CreateTestWindow()), m_ownedWindow(GetHWND())
{
}

HWND TestParentView::CreateTestWindow()
{
	HWND hwnd = CreateWindow(WC_STATIC, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
		GetModuleHandle(nullptr), nullptr);
	CHECK(hwnd);
	return hwnd;
}
