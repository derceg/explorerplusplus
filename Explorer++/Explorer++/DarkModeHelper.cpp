// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DarkModeHelper.h"
#include <wil/common.h>
#include <detours/detours.h>

DarkModeHelper::OpenNcThemeDataType DarkModeHelper::m_OpenNcThemeData = nullptr;

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

	if (osVersionInfo.dwBuildNumber == BUILD_NUMBER_1809)
	{
		m_isWindows10Version1809 = true;
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
	m_OpenNcThemeData = reinterpret_cast<OpenNcThemeDataType>(
		GetProcAddress(m_uxThemeLib.get(), MAKEINTRESOURCEA(49)));

	m_SetWindowCompositionAttribute = reinterpret_cast<SetWindowCompositionAttributeType>(
		GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));

	m_darkModeSupported = true;
}

bool DarkModeHelper::IsDarkModeSupported() const
{
	return m_darkModeSupported;
}

bool DarkModeHelper::IsDarkModeEnabled() const
{
	return m_darkModeEnabled;
}

void DarkModeHelper::EnableForApp()
{
	if (!m_darkModeSupported || IsHighContrast())
	{
		return;
	}

	if (m_isWindows10Version1809 && !m_ShouldAppsUseDarkMode())
	{
		// SetPreferredAppMode() was added in Windows 10 1903. That method allows dark mode to be
		// force enabled for an application, regardless of any system settings. In Windows 10 1809,
		// only the AllowDarkModeForApp() function exists. Whether or not dark mode is enabled for
		// the application then depends on whether it's enabled at the system level.
		// Therefore, if the application is running on Windows 10 1809, dark mode shouldn't be
		// enabled in the application unless it's enabled at the system level.
		// Without this check, enabling dark mode in the application would mean that some of the
		// controls would appear dark (where custom colors have been set on the control), but
		// controls drawn by the system (e.g. menus) would still appear light.
		// Since dark mode can be forcibly enabled for an application in later versions of Windows,
		// this check is unnecessary in those situations.
		return;
	}

	AllowDarkModeForApp(true);
	FlushMenuThemes();
	RefreshImmersiveColorPolicyState();

	[[maybe_unused]] LONG res = DetourOpenNcThemeData();
	assert(res == NO_ERROR);

	m_darkModeEnabled = true;
}

void DarkModeHelper::AllowDarkModeForApp(bool allow)
{
	if (m_SetPreferredAppMode)
	{
		m_SetPreferredAppMode(allow ? ForceDark : Default);
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

bool DarkModeHelper::ShouldAppsUseDarkMode()
{
	if (!m_ShouldAppsUseDarkMode)
	{
		return false;
	}

	return m_ShouldAppsUseDarkMode();
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

LONG DarkModeHelper::DetourOpenNcThemeData()
{
	LONG res = DetourTransactionBegin();

	if (res != NO_ERROR)
	{
		return res;
	}

	res = DetourUpdateThread(GetCurrentThread());

	if (res != NO_ERROR)
	{
		return res;
	}

	res = DetourAttach(&(PVOID &) m_OpenNcThemeData, DetouredOpenNcThemeData);

	if (res != NO_ERROR)
	{
		return res;
	}

	res = DetourTransactionCommit();

	if (res != NO_ERROR)
	{
		return res;
	}

	return res;
}

HTHEME WINAPI DarkModeHelper::DetouredOpenNcThemeData(HWND hwnd, LPCWSTR classList)
{
	// The "ItemsView" theme used to style listview controls in dark mode doesn't change the
	// scrollbar colors. By changing the class here, the scrollbars in a listview control will
	// appear dark in dark mode.
	if (lstrcmp(classList, L"ScrollBar") == 0)
	{
		hwnd = nullptr;
		classList = L"Explorer::ScrollBar";
	}

	return m_OpenNcThemeData(hwnd, classList);
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

HBRUSH DarkModeHelper::GetBackgroundBrush()
{
	if (!m_backgroundBrush)
	{
		m_backgroundBrush.reset(CreateSolidBrush(BACKGROUND_COLOR));
	}

	return m_backgroundBrush.get();
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
	TreeView_SetTextColor(treeView, TEXT_COLOR);
	TreeView_SetInsertMarkColor(treeView, FOREGROUND_COLOR);

	InvalidateRect(treeView, nullptr, TRUE);
}
