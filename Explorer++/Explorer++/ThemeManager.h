// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>
#include <unordered_set>

// Based on
// https://github.com/TortoiseGit/TortoiseGit/blob/2419d47129410d0aa371929d674bf21122c0b581/src/Utils/Theme.cpp.
class ThemeManager
{
public:
	static ThemeManager &GetInstance();

	// This will theme a top-level window, plus all of its nested children. Once a window is
	// tracked, any changes to the dark mode status will result in the window theme being
	// automatically updated.
	// Note that these methods shouldn't be called directly; instead ThemeWindowTracker should be
	// used.
	void TrackTopLevelWindow(HWND hwnd);
	void UntrackTopLevelWindow(HWND hwnd);

	// This should only be called for child windows that are dynamically created. It will theme the
	// child window (plus all nested children). Child windows that exist when the parent is
	// initialized will be covered by TrackTopLevelWindow().
	void ApplyThemeToWindowAndChildren(HWND hwnd);

private:
	static const UINT_PTR SUBCLASS_ID = 0;

	// This is the same background color as used in the Explorer address bar.
	static inline constexpr COLORREF COMBO_BOX_EX_DARK_MODE_BACKGROUND_COLOR = RGB(25, 25, 25);

	ThemeManager();

	void OnDarkModeStatusChanged();

	static void ApplyThemeToWindow(HWND hwnd);
	static BOOL CALLBACK ProcessChildWindow(HWND hwnd, LPARAM lParam);
	static BOOL CALLBACK ProcessThreadWindow(HWND hwnd, LPARAM lParam);
	static void ApplyThemeToMainWindow(HWND hwnd, bool enableDarkMode);
	static void ApplyThemeToDialog(HWND hwnd, bool enableDarkMode);
	static void ApplyThemeToListView(HWND hwnd, bool enableDarkMode);
	static void ApplyThemeToHeader(HWND hwnd);
	static void ApplyThemeToTreeView(HWND hwnd, bool enableDarkMode);
	static void ApplyThemeToRichEdit(HWND hwnd, bool enableDarkMode);
	static void ApplyThemeToRebar(HWND hwnd, bool enableDarkMode);
	static void ApplyThemeToToolbar(HWND hwnd, bool enableDarkMode);
	static void ApplyThemeToComboBoxEx(HWND hwnd, bool enableDarkMode);
	static void ApplyThemeToComboBox(HWND hwnd);
	static void ApplyThemeToButton(HWND hwnd, bool enableDarkMode);
	static void ApplyThemeToTooltips(HWND hwnd);
	static void ApplyThemeToStatusBar(HWND hwnd, bool enableDarkMode);
	static void ApplyThemeToScrollBar(HWND hwnd, bool enableDarkMode);

	static LRESULT CALLBACK MainWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
		UINT_PTR subclassId, DWORD_PTR data);
	static HBRUSH GetMenuBarBackgroundBrush(bool enableDarkMode);
	static bool ShouldAlwaysShowAccessKeys();
	static LRESULT CALLBACK DialogSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
		UINT_PTR subclassId, DWORD_PTR data);
	static LRESULT CALLBACK ToolbarParentSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
		UINT_PTR subclassId, DWORD_PTR data);
	static LRESULT OnCustomDraw(NMCUSTOMDRAW *customDraw);
	static LRESULT OnButtonCustomDraw(NMCUSTOMDRAW *customDraw);
	static LRESULT OnToolbarCustomDraw(NMTBCUSTOMDRAW *customDraw);
	static LRESULT CALLBACK ComboBoxExSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
		UINT_PTR subclassId, DWORD_PTR data);
	static HBRUSH GetComboBoxExBackgroundBrush();
	static LRESULT CALLBACK ListViewSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
		UINT_PTR subclassId, DWORD_PTR data);
	static LRESULT CALLBACK RebarSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
		UINT_PTR subclassId, DWORD_PTR data);
	static LRESULT CALLBACK GroupBoxSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
		UINT_PTR subclassId, DWORD_PTR data);
	static LRESULT CALLBACK ScrollBarSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
		UINT_PTR subclassId, DWORD_PTR data);

	std::unordered_set<HWND> m_trackedTopLevelWindows;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
