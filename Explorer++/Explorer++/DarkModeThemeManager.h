// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

// Based on
// https://github.com/TortoiseGit/TortoiseGit/blob/2419d47129410d0aa371929d674bf21122c0b581/src/Utils/Theme.cpp.
class DarkModeThemeManager
{
public:
	static DarkModeThemeManager &GetInstance();

	void ApplyThemeToTopLevelWindow(HWND topLevelWindow);

private:
	static const UINT_PTR SUBCLASS_ID = 0;

	DarkModeThemeManager() = default;

	static BOOL CALLBACK ProcessChildWindow(HWND hwnd, LPARAM lParam);
	static void ApplyThemeToWindow(HWND hwnd);

	static LRESULT CALLBACK DialogSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
		UINT_PTR subclassId, DWORD_PTR data);
	static LRESULT CALLBACK ToolbarParentSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
		UINT_PTR subclassId, DWORD_PTR data);
	static LRESULT OnCustomDraw(NMCUSTOMDRAW *customDraw);
	static LRESULT OnButtonCustomDraw(NMCUSTOMDRAW *customDraw);
	static LRESULT OnToolbarCustomDraw(NMTBCUSTOMDRAW *customDraw);
	static LRESULT CALLBACK ListViewSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
		UINT_PTR subclassId, DWORD_PTR data);
};
