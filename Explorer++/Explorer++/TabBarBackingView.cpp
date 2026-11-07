// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabBarBackingView.h"
#include "DarkModeColorProvider.h"
#include "DarkModeManager.h"
#include "TabBarBackingViewDelegate.h"
#include "../Helper/WindowSubclass.h"
#include <memory>

TabBarBackingView::TabBarBackingView(ViewBase *parent, const DarkModeManager *darkModeManager,
	const DarkModeColorProvider *darkModeColorProvider) :
	ViewBase(CreateBacking(parent)),
	m_darkModeManager(darkModeManager),
	m_darkModeColorProvider(darkModeColorProvider)
{
	AddSubclass(std::make_unique<WindowSubclass>(GetHWND(),
		std::bind_front(&TabBarBackingView::WndProc, this)));
}

HWND TabBarBackingView::CreateBacking(ViewBase *parent)
{
	static bool classRegistered = false;

	if (!classRegistered)
	{
		auto res = RegisterBackingClass();
		CHECK_NE(res, 0);

		classRegistered = true;
	}

	HWND hwnd = CreateWindow(CLASS_NAME, L"", WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, 0,
		0, parent->GetHWND(), nullptr, GetModuleHandle(nullptr), nullptr);
	CHECK(hwnd);
	return hwnd;
}

ATOM TabBarBackingView::RegisterBackingClass()
{
	WNDCLASS windowClass = {};
	windowClass.lpfnWndProc = DefWindowProc;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.lpszClassName = CLASS_NAME;
	windowClass.hInstance = GetModuleHandle(nullptr);
	windowClass.style = CS_DBLCLKS;
	return RegisterClass(&windowClass);
}

void TabBarBackingView::SetDelegate(TabBarBackingViewDelegate *delegate)
{
	m_delegate = delegate;
}

LRESULT TabBarBackingView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_LBUTTONDBLCLK:
		OnDoubleClick();
		break;

	case WM_RBUTTONUP:
		OnRButtonUp({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
		break;

	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT:
		OnPaint();
		return 0;

	case WM_PRINTCLIENT:
		OnPrintClient(reinterpret_cast<HDC>(wParam));
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void TabBarBackingView::OnDoubleClick()
{
	if (!m_delegate)
	{
		return;
	}

	m_delegate->OnDoubleClicked();
}

void TabBarBackingView::OnRButtonUp(const POINT &ptClient)
{
	if (!m_delegate)
	{
		return;
	}

	POINT ptScreen = ptClient;
	auto res = ClientToScreen(GetHWND(), &ptScreen);
	CHECK(res);

	m_delegate->OnShowContextMenu(ptScreen);
}

void TabBarBackingView::OnPaint()
{
	PAINTSTRUCT ps;
	BeginPaint(GetHWND(), &ps);
	PerformPaint(ps);
	EndPaint(GetHWND(), &ps);
}

void TabBarBackingView::OnPrintClient(HDC hdc)
{
	PAINTSTRUCT ps = {};
	ps.hdc = hdc;
	auto res = GetClientRect(GetHWND(), &ps.rcPaint);
	CHECK(res);
	PerformPaint(ps);
}

void TabBarBackingView::PerformPaint(const PAINTSTRUCT &ps)
{
	HBRUSH backgroundBrush;

	if (m_darkModeManager->IsDarkModeEnabled())
	{
		backgroundBrush = m_darkModeColorProvider->GetTabBarBackgroundBrush();
	}
	else
	{
		backgroundBrush = GetSysColorBrush(COLOR_BTNFACE);
	}

	auto res = FillRect(ps.hdc, &ps.rcPaint, backgroundBrush);
	DCHECK(res);
}
