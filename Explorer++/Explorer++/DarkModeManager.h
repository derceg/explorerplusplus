// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/SignalWrapper.h"
#include <wil/resource.h>
#include <memory>

struct Config;
class EventWindow;

// Based on the code contained within:
// https://github.com/ysc3839/win32-darkmode/blob/bb241c369fee7b56440420179654bb487f7259cd/win32-darkmode/DarkMode.h

// This class allows dark mode to be enabled and disabled. An instance of this class is designed to
// be held for the lifetime of the application. If dark mode is enabled when the instance is
// destroyed, no attempt will be made to disable it.
class DarkModeManager
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

	static constexpr COLORREF BACKGROUND_COLOR = RGB(32, 32, 32);
	static constexpr COLORREF TEXT_COLOR = RGB(255, 255, 255);

	// The text color used for items that are active, but considered to be in the background. For
	// example, in a tab control, a single item is selected. The text for the other tabs could be
	// drawn in this color to visually distinguish them from the selected tab.
	static constexpr COLORREF TEXT_COLOR_BACKGROUND = RGB(180, 180, 180);

	static constexpr COLORREF TEXT_COLOR_DISABLED = RGB(121, 121, 121);

	// The color of foreground elements (e.g. the toolbar insertion mark).
	static constexpr COLORREF FOREGROUND_COLOR = RGB(255, 255, 255);

	// The background color of the selected item.
	static constexpr COLORREF SELECTED_ITEM_BACKGROUND_COLOR = RGB(100, 100, 100);

	// The background color of the hot item (i.e. the item that's under the mouse).
	static constexpr COLORREF HOT_ITEM_BACKGROUND_COLOR = RGB(71, 71, 71);

	// The color of borders (e.g. the borders around an individual tab).
	static constexpr COLORREF BORDER_COLOR = RGB(120, 120, 120);

	DarkModeManager(EventWindow *eventWindow, const Config *config);

	bool IsDarkModeSupported() const;
	bool IsDarkModeEnabled() const;

	void AllowDarkModeForWindow(HWND hwnd, bool allow);

	static bool IsHighContrast();

	HBRUSH GetBackgroundBrush() const;
	HBRUSH GetSelectedItemBackgroundBrush() const;
	HBRUSH GetHotItemBackgroundBrush() const;
	HBRUSH GetBorderBrush() const;

	SignalWrapper<DarkModeManager, void(bool darkModeEnabled)> darkModeStatusChanged;

private:
	static constexpr DWORD BUILD_NUMBER_1809 = 17763;
	static constexpr DWORD BUILD_NUMBER_1903 = 18362;
	static constexpr DWORD BUILD_NUMBER_1909 = 18363;

	using RtlGetVersionType = NTSTATUS(WINAPI *)(LPOSVERSIONINFOEXW);

	// Windows 10 1809
	using AllowDarkModeForAppType = bool(WINAPI *)(bool allow);
	using FlushMenuThemesType = void(WINAPI *)();
	using RefreshImmersiveColorPolicyStateType = void(WINAPI *)();
	using AllowDarkModeForWindowType = bool(WINAPI *)(HWND hwnd, bool allow);
	using OpenNcThemeDataType = HTHEME(WINAPI *)(HWND hwnd, LPCWSTR classList);

	// Windows 10 1903
	using SetPreferredAppModeType = PreferredAppMode(WINAPI *)(PreferredAppMode appMode);

	void OnEventWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnSettingChange(const wchar_t *systemParameter);
	void OnThemeUpdated();
	void UpdateAppDarkModeStatus();
	bool ShouldEnableDarkMode() const;
	bool IsSystemAppModeLight() const;

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

	// Windows 10 1903
	SetPreferredAppModeType m_SetPreferredAppMode = nullptr;

	// This is static, as it needs to be called by DetouredOpenNcThemeData(). The only two options
	// here are a static variable or a global variable.
	static inline OpenNcThemeDataType m_OpenNcThemeData = nullptr;

	const Config *const m_config;
	wil::unique_hmodule m_uxThemeLib;
	bool m_isWindows10Version1809 = false;
	bool m_darkModeSupported = false;
	bool m_darkModeEnabled = false;
	const wil::unique_hbrush m_backgroundBrush;
	const wil::unique_hbrush m_selectedItemBackgroundBrush;
	const wil::unique_hbrush m_hotItemBackgroundBrush;
	const wil::unique_hbrush m_borderBrush;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
