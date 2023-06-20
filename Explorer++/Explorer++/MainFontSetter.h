// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "SignalWrapper.h"

struct Config;
class WindowSubclassWrapper;

// Applies the main font (if any) to a window. If the main font is changed or reset to default, the
// window font will be updated. The lifetime of this class should be the same as the lifetime of the
// window itself, since the class manages the font assigned to the window and that font needs to
// have the same lifetime as the window.
class MainFontSetter
{
public:
	MainFontSetter(HWND hwnd, const Config *config);

	// Signals
	SignalWrapper<MainFontSetter, void()> fontUpdatedSignal;

private:
	void MaybeSubclassWindow();
	LRESULT TooltipWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void UpdateFont();

	HWND m_hwnd;
	const Config *const m_config;
	wil::unique_hfont m_font;

	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
