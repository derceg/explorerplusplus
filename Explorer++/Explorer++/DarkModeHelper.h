// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>

// Based on the code contained within:
// https://github.com/ysc3839/win32-darkmode/blob/bb241c369fee7b56440420179654bb487f7259cd/win32-darkmode/DarkMode.h

class DarkModeHelper
{
public:
	enum PreferredAppMode
	{
		Default,
		AllowDark,
		ForceDark,
		ForceLight,
		Max
	};

	enum WINDOWCOMPOSITIONATTRIB
	{
		WCA_UNDEFINED = 0,
		WCA_NCRENDERING_ENABLED = 1,
		WCA_NCRENDERING_POLICY = 2,
		WCA_TRANSITIONS_FORCEDISABLED = 3,
		WCA_ALLOW_NCPAINT = 4,
		WCA_CAPTION_BUTTON_BOUNDS = 5,
		WCA_NONCLIENT_RTL_LAYOUT = 6,
		WCA_FORCE_ICONIC_REPRESENTATION = 7,
		WCA_EXTENDED_FRAME_BOUNDS = 8,
		WCA_HAS_ICONIC_BITMAP = 9,
		WCA_THEME_ATTRIBUTES = 10,
		WCA_NCRENDERING_EXILED = 11,
		WCA_NCADORNMENTINFO = 12,
		WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
		WCA_VIDEO_OVERLAY_ACTIVE = 14,
		WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
		WCA_DISALLOW_PEEK = 16,
		WCA_CLOAK = 17,
		WCA_CLOAKED = 18,
		WCA_ACCENT_POLICY = 19,
		WCA_FREEZE_REPRESENTATION = 20,
		WCA_EVER_UNCLOAKED = 21,
		WCA_VISUAL_OWNER = 22,
		WCA_HOLOGRAPHIC = 23,
		WCA_EXCLUDED_FROM_DDA = 24,
		WCA_PASSIVEUPDATEMODE = 25,
		WCA_USEDARKMODECOLORS = 26,
		WCA_LAST = 27
	};

	struct WINDOWCOMPOSITIONATTRIBDATA
	{
		WINDOWCOMPOSITIONATTRIB Attrib;
		PVOID pvData;
		SIZE_T cbData;
	};

	static inline constexpr COLORREF BACKGROUND_COLOR = RGB(32, 32, 32);
	static inline constexpr COLORREF FOREGROUND_COLOR = RGB(255, 255, 255);
	static inline constexpr COLORREF BUTTON_HIGHLIGHT_COLOR = RGB(71, 71, 71);

	static DarkModeHelper &GetInstance();

	void EnableForApp();

	void AllowDarkModeForApp(bool allow);
	void FlushMenuThemes();
	void RefreshImmersiveColorPolicyState();
	void AllowDarkModeForWindow(HWND hWnd, bool allow);
	void SetWindowCompositionAttribute(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA *data);
	void SetDarkModeForToolbarTooltips(HWND toolbar);
	void SetDarkModeForControl(HWND control);
	void SetDarkModeForComboBox(HWND comboBox);
	void SetListViewDarkModeColors(HWND listView);
	void SetTreeViewDarkModeColors(HWND treeView);

	bool IsDarkModeEnabled() const;

	HBRUSH GetBackgroundBrush();

private:
	static inline const DWORD BUILD_NUMBER_1809 = 17763;
	static inline const DWORD BUILD_NUMBER_1903 = 18362;
	static inline const DWORD BUILD_NUMBER_1909 = 18363;

	using RtlGetVersionType = NTSTATUS(WINAPI *)(LPOSVERSIONINFOEXW);
	using SetWindowCompositionAttributeType = BOOL(WINAPI *)(
		HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA *data);

	// Windows 10 1809
	using AllowDarkModeForAppType = BOOL(WINAPI *)(BOOL allow);
	using ShouldAppsUseDarkModeType = BOOL(WINAPI *)();
	using FlushMenuThemesType = void(WINAPI *)();
	using RefreshImmersiveColorPolicyStateType = void(WINAPI *)();
	using AllowDarkModeForWindowType = BOOL(WINAPI *)(HWND hWnd, BOOL allow);

	// Windows 10 1903
	using SetPreferredAppModeType = PreferredAppMode(WINAPI *)(PreferredAppMode appMode);

	DarkModeHelper();

	static bool IsHighContrast();

	// Windows 10 1809
	AllowDarkModeForAppType m_AllowDarkModeForApp = nullptr;
	ShouldAppsUseDarkModeType m_ShouldAppsUseDarkMode = nullptr;
	FlushMenuThemesType m_FlushMenuThemes = nullptr;
	RefreshImmersiveColorPolicyStateType m_RefreshImmersiveColorPolicyState = nullptr;
	AllowDarkModeForWindowType m_AllowDarkModeForWindow = nullptr;
	SetWindowCompositionAttributeType m_SetWindowCompositionAttribute = nullptr;

	// Windows 10 1903
	SetPreferredAppModeType m_SetPreferredAppMode = nullptr;

	wil::unique_hmodule m_uxThemeLib;
	bool m_darkModeSupported;
	bool m_darkModeEnabled;
	wil::unique_hbrush m_backgroundBrush;
};