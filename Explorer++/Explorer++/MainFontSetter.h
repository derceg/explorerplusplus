// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "SignalWrapper.h"
#include <optional>

struct Config;
class WindowSubclass;

// Applies the main font (if any) to a window. If the main font is changed or reset to default, the
// window font will be updated. The lifetime of this class should be the same as the lifetime of the
// window itself, since the class manages the font assigned to the window and that font needs to
// have the same lifetime as the window.
class MainFontSetter
{
public:
	MainFontSetter(HWND hwnd, const Config *config,
		std::optional<LOGFONT> defaultFontAt96Dpi = std::nullopt);
	~MainFontSetter();

	// Signals
	SignalWrapper<MainFontSetter, void()> fontUpdatedSignal;

private:
	void SubclassWindowForDpiChanges();
	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnDpiChanged();
	void MaybeSubclassSpecificWindowClasses();
	LRESULT TooltipWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void UpdateFont();

	HWND m_hwnd;
	const Config *const m_config;
	wil::unique_hfont m_font;
	const std::optional<LOGFONT> m_defaultFontAt96Dpi;

	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
