// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "TabBackingToolbarView.h"
#include "ViewBase.h"

struct Config;
class DarkModeColorProvider;
class DarkModeManager;
class MenuHelpTextHost;
class TabBackingViewDelegate;

class TabBackingView : public ViewBase
{
public:
	TabBackingView(ViewBase *parent, MenuHelpTextHost *menuHelpTextHost,
		const DarkModeManager *darkModeManager, const DarkModeColorProvider *darkModeColorProvider,
		const Config *config);

	void SetDelegate(TabBackingViewDelegate *delegate);
	TabBackingToolbarView *GetToolbarView();

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
	void UpdateLayout();

	TabBackingToolbarView m_toolbarView;
	const DarkModeManager *const m_darkModeManager;
	const DarkModeColorProvider *const m_darkModeColorProvider;
	TabBackingViewDelegate *m_delegate = nullptr;
};
