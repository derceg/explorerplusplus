// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

// Based on
// https://github.com/TortoiseGit/TortoiseGit/blob/2419d47129410d0aa371929d674bf21122c0b581/src/Utils/Theme.cpp.
class ThemeManager
{
public:
	static ThemeManager &GetInstance();

	void ApplyThemeToWindowAndChildren(HWND topLevelWindow);
	static void ApplyThemeToWindow(HWND hwnd);

private:
	static const UINT_PTR SUBCLASS_ID = 0;

	// This is the same background color as used in the Explorer address bar.
	static inline constexpr COLORREF COMBO_BOX_EX_DARK_MODE_BACKGROUND_COLOR = RGB(25, 25, 25);

	ThemeManager() = default;

	static BOOL CALLBACK ProcessChildWindow(HWND hwnd, LPARAM lParam);
	static BOOL CALLBACK ProcessThreadWindow(HWND hwnd, LPARAM lParam);
	static void ApplyThemeToMainWindow(HWND hwnd, bool enableDarkMode);
	static void ApplyThemeToDialog(HWND hwnd, bool enableDarkMode);
	static void ApplyThemeToListView(HWND hwnd, bool enableDarkMode);
	static void ApplyThemeToHeader(HWND hwnd, bool enableDarkMode);
	static void ApplyThemeToTreeView(HWND hwnd, bool enableDarkMode);
	static void ApplyThemeToRichEdit(HWND hwnd, bool enableDarkMode);
	static void ApplyThemeToRebar(HWND hwnd, bool enableDarkMode);
	static void ApplyThemeToToolbar(HWND hwnd, bool enableDarkMode);
	static void ApplyThemeToComboBoxEx(HWND hwnd, bool enableDarkMode);
	static void ApplyThemeToComboBox(HWND hwnd, bool enableDarkMode);
	static void ApplyThemeToButton(HWND hwnd, bool enableDarkMode);
	static void ApplyThemeToTooltips(HWND hwnd, bool enableDarkMode);
	static void ApplyThemeToStatusBar(HWND hwnd, bool enableDarkMode);

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
};
