// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DarkModeHelper.h"
#include <wil/common.h>

DarkModeHelper &DarkModeHelper::GetInstance()
{
	static DarkModeHelper darkModeHelper;
	return darkModeHelper;
}

DarkModeHelper::DarkModeHelper() : m_darkModeSupported(false), m_darkModeEnabled(false)
{
	auto RtlGetVersion = reinterpret_cast<RtlGetVersionType>(
		GetProcAddress(GetModuleHandle(L"ntdll.dll"), "RtlGetVersion"));

	if (!RtlGetVersion)
	{
		return;
	}

	OSVERSIONINFOEX osVersionInfo;
	osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
	RtlGetVersion(&osVersionInfo);

	if (osVersionInfo.dwMajorVersion != 10 || osVersionInfo.dwMinorVersion != 0
		|| osVersionInfo.dwBuildNumber < BUILD_NUMBER_1809)
	{
		return;
	}

	m_uxThemeLib.reset(LoadLibrary(_T("uxtheme.dll")));

	if (!m_uxThemeLib)
	{
		return;
	}

	if (osVersionInfo.dwBuildNumber < BUILD_NUMBER_1903)
	{
		m_AllowDarkModeForApp = reinterpret_cast<AllowDarkModeForAppType>(
			GetProcAddress(m_uxThemeLib.get(), MAKEINTRESOURCEA(135)));
	}
	else
	{
		m_SetPreferredAppMode = reinterpret_cast<SetPreferredAppModeType>(
			GetProcAddress(m_uxThemeLib.get(), MAKEINTRESOURCEA(135)));
	}

	m_ShouldAppsUseDarkMode = reinterpret_cast<ShouldAppsUseDarkModeType>(
		GetProcAddress(m_uxThemeLib.get(), MAKEINTRESOURCEA(132)));
	m_FlushMenuThemes = reinterpret_cast<FlushMenuThemesType>(
		GetProcAddress(m_uxThemeLib.get(), MAKEINTRESOURCEA(136)));
	m_RefreshImmersiveColorPolicyState = reinterpret_cast<RefreshImmersiveColorPolicyStateType>(
		GetProcAddress(m_uxThemeLib.get(), MAKEINTRESOURCEA(104)));
	m_AllowDarkModeForWindow = reinterpret_cast<AllowDarkModeForWindowType>(
		GetProcAddress(m_uxThemeLib.get(), MAKEINTRESOURCEA(133)));

	m_SetWindowCompositionAttribute = reinterpret_cast<SetWindowCompositionAttributeType>(
		GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));

	m_darkModeSupported = true;
}

void DarkModeHelper::EnableForApp()
{
	AllowDarkModeForApp(true);
	FlushMenuThemes();
	RefreshImmersiveColorPolicyState();

	m_darkModeEnabled = m_ShouldAppsUseDarkMode() && !IsHighContrast();
}

void DarkModeHelper::AllowDarkModeForApp(bool allow)
{
	if (m_SetPreferredAppMode)
	{
		m_SetPreferredAppMode(allow ? AllowDark : Default);
	}
	else if (m_AllowDarkModeForApp)
	{
		m_AllowDarkModeForApp(allow);
	}
}

void DarkModeHelper::AllowDarkModeForWindow(HWND hWnd, bool allow)
{
	if (m_AllowDarkModeForWindow)
	{
		m_AllowDarkModeForWindow(hWnd, allow);
	}
}

void DarkModeHelper::FlushMenuThemes()
{
	if (m_FlushMenuThemes)
	{
		m_FlushMenuThemes();
	}
}

void DarkModeHelper::RefreshImmersiveColorPolicyState()
{
	if (m_RefreshImmersiveColorPolicyState)
	{
		m_RefreshImmersiveColorPolicyState();
	}
}

void DarkModeHelper::SetWindowCompositionAttribute(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA *data)
{
	if (m_SetWindowCompositionAttribute)
	{
		m_SetWindowCompositionAttribute(hWnd, data);
	}
}

bool DarkModeHelper::IsHighContrast()
{
	HIGHCONTRAST highContrast;
	BOOL res =
		SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(highContrast), &highContrast, FALSE);

	if (!res)
	{
		return false;
	}

	return WI_IsFlagSet(highContrast.dwFlags, HCF_HIGHCONTRASTON);
}

void DarkModeHelper::SetDarkModeForToolbarTooltips(HWND toolbar)
{
	HWND tooltips = reinterpret_cast<HWND>(SendMessage(toolbar, TB_GETTOOLTIPS, 0, 0));

	if (tooltips)
	{
		SetDarkModeForControl(tooltips);
	}
}

// Sets the dark mode theme for a control. Works for controls that use the DarkMode_Explorer theme:
// 
// - Buttons
// - Scrollbars
// - TreeViews
// - Tooltips
// - Edit controls
void DarkModeHelper::SetDarkModeForControl(HWND control)
{
	AllowDarkModeForWindow(control, true);
	SetWindowTheme(control, L"Explorer", nullptr);
}

void DarkModeHelper::SetDarkModeForComboBox(HWND comboBox)
{
	AllowDarkModeForWindow(comboBox, true);
	SetWindowTheme(comboBox, L"CFD", nullptr);
}

bool DarkModeHelper::IsDarkModeEnabled() const
{
	return m_darkModeEnabled;
}

HBRUSH DarkModeHelper::GetBackgroundBrush()
{
	if (!m_backgroundBrush)
	{
		m_backgroundBrush.reset(CreateSolidBrush(BACKGROUND_COLOR));
	}

	return m_backgroundBrush.get();
}

void DarkModeHelper::SetListViewDarkModeColors(HWND listView)
{
	AllowDarkModeForWindow(listView, true);
	SetWindowTheme(listView, L"ItemsView", nullptr);

	HWND header = ListView_GetHeader(listView);
	AllowDarkModeForWindow(header, true);
	SetWindowTheme(header, L"ItemsView", nullptr);

	ListView_SetBkColor(listView, BACKGROUND_COLOR);
	ListView_SetTextBkColor(listView, BACKGROUND_COLOR);
	ListView_SetTextColor(listView, FOREGROUND_COLOR);

	InvalidateRect(listView, nullptr, TRUE);
}

void DarkModeHelper::SetTreeViewDarkModeColors(HWND treeView)
{
	AllowDarkModeForWindow(treeView, true);

	// When in dark mode, this theme sets the following colors correctly:
	// 
	// - the item selection color,
	// - the colors of the arrows that appear to the left of the items,
	// - the color of the scrollbars.
	// 
	// It doesn't, however, change the background color, or the text color.
	SetWindowTheme(treeView, L"Explorer", nullptr);

	TreeView_SetBkColor(treeView, BACKGROUND_COLOR);
	TreeView_SetTextColor(treeView, FOREGROUND_COLOR);

	InvalidateRect(treeView, nullptr, TRUE);
}