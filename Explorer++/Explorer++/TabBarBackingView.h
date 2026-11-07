// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ViewBase.h"

class TabBarBackingViewDelegate;
class ThemeManager;

class TabBarBackingView : public ViewBase
{
public:
	TabBarBackingView(ViewBase *parent, const ThemeManager *themeManager);

	void SetDelegate(TabBarBackingViewDelegate *delegate);

private:
	static constexpr wchar_t CLASS_NAME[] = L"Explorer++TabBackingWindowClass";

	// The distance between the toolbar and the right edge of the window.
	static constexpr int TOOLBAR_X_OFFSET = 4;

	static HWND CreateBacking(ViewBase *parent);
	static ATOM RegisterBackingClass();

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnDoubleClick();
	void OnRButtonUp(const POINT &ptClient);
	void OnPaint();
	void OnPrintClient(HDC hdc);
	void PerformPaint(const PAINTSTRUCT &ps);

	const ThemeManager *const m_themeManager;
	TabBarBackingViewDelegate *m_delegate = nullptr;
};
