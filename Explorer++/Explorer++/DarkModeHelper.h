// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "SignalWrapper.h"
#include <wil/resource.h>

// Based on the code contained within:
// https://github.com/ysc3839/win32-darkmode/blob/bb241c369fee7b56440420179654bb487f7259cd/win32-darkmode/DarkMode.h

// This class allows dark mode to be enabled and disabled. An instance of this class is designed to
// be held for the lifetime of the application. If dark mode is enabled when the instance is
// destroyed, no attempt will be made to disable it.
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

	static constexpr COLORREF BACKGROUND_COLOR = RGB(32, 32, 32);
	static constexpr COLORREF TEXT_COLOR = RGB(255, 255, 255);

	static constexpr COLORREF TEXT_COLOR_DISABLED = RGB(121, 121, 121);

	// The color of foreground elements (e.g. the toolbar insertion mark).
	static constexpr COLORREF FOREGROUND_COLOR = RGB(255, 255, 255);

	// The color of the hot item (i.e. the item that's selected/under the mouse).
	static constexpr COLORREF HOT_ITEM_HIGHLIGHT_COLOR = RGB(71, 71, 71);

	DarkModeHelper();

	bool IsDarkModeSupported() const;
	bool IsDarkModeEnabled() const;

	void EnableForApp(bool enable);

	bool IsSystemAppModeLight();
	void AllowDarkModeForWindow(HWND hwnd, bool allow);
	void SetWindowCompositionAttribute(HWND hwnd, WINDOWCOMPOSITIONATTRIBDATA *data);

	static bool IsHighContrast();

	HBRUSH GetBackgroundBrush() const;

	SignalWrapper<DarkModeHelper, void(bool darkModeEnabled)> darkModeStatusChanged;

private:
	static constexpr DWORD BUILD_NUMBER_1809 = 17763;
	static constexpr DWORD BUILD_NUMBER_1903 = 18362;
	static constexpr DWORD BUILD_NUMBER_1909 = 18363;

	using RtlGetVersionType = NTSTATUS(WINAPI *)(LPOSVERSIONINFOEXW);
	using SetWindowCompositionAttributeType = BOOL(
		WINAPI *)(HWND hwnd, WINDOWCOMPOSITIONATTRIBDATA *data);

	// Windows 10 1809
	using AllowDarkModeForAppType = bool(WINAPI *)(bool allow);
	using FlushMenuThemesType = void(WINAPI *)();
	using RefreshImmersiveColorPolicyStateType = void(WINAPI *)();
	using AllowDarkModeForWindowType = bool(WINAPI *)(HWND hwnd, bool allow);
	using OpenNcThemeDataType = HTHEME(WINAPI *)(HWND hwnd, LPCWSTR classList);

	// Windows 10 1903
	using SetPreferredAppModeType = PreferredAppMode(WINAPI *)(PreferredAppMode appMode);

	void AllowDarkModeForApp(bool allow);
	void FlushMenuThemes();
	void RefreshImmersiveColorPolicyState();

	LONG DetourOpenNcThemeData();
	LONG RestoreOpenNcThemeData();
	static HTHEME WINAPI DetouredOpenNcThemeData(HWND hwnd, LPCWSTR classList);

	// Windows 10 1809
	AllowDarkModeForAppType m_AllowDarkModeForApp = nullptr;
	FlushMenuThemesType m_FlushMenuThemes = nullptr;
	RefreshImmersiveColorPolicyStateType m_RefreshImmersiveColorPolicyState = nullptr;
	AllowDarkModeForWindowType m_AllowDarkModeForWindow = nullptr;
	SetWindowCompositionAttributeType m_SetWindowCompositionAttribute = nullptr;

	// Windows 10 1903
	SetPreferredAppModeType m_SetPreferredAppMode = nullptr;

	// This is static, as it needs to be called by DetouredOpenNcThemeData(). The only two options
	// here are a static variable or a global variable.
	static inline OpenNcThemeDataType m_OpenNcThemeData = nullptr;

	wil::unique_hmodule m_uxThemeLib;
	bool m_isWindows10Version1809 = false;
	bool m_darkModeSupported = false;
	bool m_darkModeEnabled = false;
	wil::unique_hbrush m_backgroundBrush;
};
