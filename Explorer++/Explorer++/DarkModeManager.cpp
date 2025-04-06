// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DarkModeManager.h"
#include "Config.h"
#include "EventWindow.h"
#include "../Helper/DetoursHelper.h"
#include "../Helper/Helper.h"
#include "../Helper/RegistrySettings.h"
#include <detours/detours.h>
#include <wil/common.h>

DarkModeManager::DarkModeManager(EventWindow *eventWindow, const Config *config) :
	m_config(config),
	m_backgroundBrush(CreateSolidBrush(BACKGROUND_COLOR))
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

	m_uxThemeLib = LoadSystemLibrary(L"uxtheme.dll");

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

	m_connections.push_back(eventWindow->windowMessageSignal.AddObserver(
		std::bind_front(&DarkModeManager::OnEventWindowMessage, this)));

	m_connections.push_back(
		m_config->theme.addObserver(std::bind(&DarkModeManager::OnThemeUpdated, this)));

	m_darkModeSupported = true;

	UpdateAppDarkModeStatus();
}

bool DarkModeManager::IsDarkModeSupported() const
{
	return m_darkModeSupported;
}

bool DarkModeManager::IsDarkModeEnabled() const
{
	return m_darkModeEnabled;
}

void DarkModeManager::OnEventWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(wParam);

	switch (msg)
	{
	case WM_SETTINGCHANGE:
		OnSettingChange(reinterpret_cast<const wchar_t *>(lParam));
		break;
	}
}

void DarkModeManager::OnSettingChange(const wchar_t *systemParameter)
{
	// The "ImmersiveColorSet" change notification will be sent when the user changes the dark mode
	// setting in Windows (or one of the individual Windows mode/app mode settings). Changes to the
	// Windows mode setting will be ignored, as the app mode setting is what's used to determine
	// whether a light or dark theme is used.
	if (lstrcmp(systemParameter, L"ImmersiveColorSet") == 0
		&& m_config->theme.get() == +Theme::System)
	{
		UpdateAppDarkModeStatus();
	}
}

void DarkModeManager::OnThemeUpdated()
{
	UpdateAppDarkModeStatus();
}

void DarkModeManager::UpdateAppDarkModeStatus()
{
	bool enable = ShouldEnableDarkMode();

	if (!m_darkModeSupported || IsHighContrast() || enable == m_darkModeEnabled)
	{
		return;
	}

	if (m_isWindows10Version1809 && IsSystemAppModeLight() && enable)
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

	AllowDarkModeForApp(enable);
	FlushMenuThemes();
	RefreshImmersiveColorPolicyState();

	// This detour seemingly causes an illegal instruction (0xc000001d) crash on ARM64 (see #478),
	// which is the reason for the check here. Note that while disabling this detour on ARM64
	// prevents the crash, it also means some scrollbars (e.g. the listview scrollbar) will appear
	// light when dark mode is enabled on ARM64.
#if !defined(BUILD_ARM64)
	LONG res;

	if (enable)
	{
		res = DetourOpenNcThemeData();
	}
	else
	{
		res = RestoreOpenNcThemeData();
	}

	DCHECK_EQ(res, NO_ERROR);
#endif

	m_darkModeEnabled = enable;

	darkModeStatusChanged.m_signal(enable);
}

bool DarkModeManager::ShouldEnableDarkMode() const
{
	return m_config->theme == +Theme::Dark
		|| (m_config->theme == +Theme::System && !IsSystemAppModeLight());
}

void DarkModeManager::AllowDarkModeForApp(bool allow)
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

void DarkModeManager::AllowDarkModeForWindow(HWND hwnd, bool allow)
{
	if (m_AllowDarkModeForWindow)
	{
		m_AllowDarkModeForWindow(hwnd, allow);
	}
}

// Note that uxtheme.dll!ShouldAppsUseDarkMode() doesn't just return the system setting. It also
// checks whether the current app mode is set to ForceLight (in which case it will return false) or
// ForceDark (in which case it will return true). That's the reason this function exists. It will
// always return the value of the system setting, independently of whatever app mode is currently
// set.
// See https://bugs.eclipse.org/bugs/show_bug.cgi?id=549713 for some further information about the
// registry key used here.
bool DarkModeManager::IsSystemAppModeLight() const
{
	bool lightMode = true;

	DWORD value;
	LSTATUS result = RegistrySettings::ReadDword(HKEY_CURRENT_USER,
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme",
		value);

	if (result == ERROR_SUCCESS)
	{
		lightMode = value;
	}

	return lightMode;
}

void DarkModeManager::FlushMenuThemes()
{
	if (m_FlushMenuThemes)
	{
		m_FlushMenuThemes();
	}
}

void DarkModeManager::RefreshImmersiveColorPolicyState()
{
	if (m_RefreshImmersiveColorPolicyState)
	{
		m_RefreshImmersiveColorPolicyState();
	}
}

LONG DarkModeManager::DetourOpenNcThemeData()
{
	return DetourTransaction(
		[] { return DetourAttach(&(PVOID &) m_OpenNcThemeData, DetouredOpenNcThemeData); });
}

LONG DarkModeManager::RestoreOpenNcThemeData()
{
	return DetourTransaction(
		[] { return DetourDetach(&(PVOID &) m_OpenNcThemeData, DetouredOpenNcThemeData); });
}

HTHEME WINAPI DarkModeManager::DetouredOpenNcThemeData(HWND hwnd, LPCWSTR classList)
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

void DarkModeManager::SetWindowCompositionAttribute(HWND hwnd, WINDOWCOMPOSITIONATTRIBDATA *data)
{
	if (m_SetWindowCompositionAttribute)
	{
		m_SetWindowCompositionAttribute(hwnd, data);
	}
}

bool DarkModeManager::IsHighContrast()
{
	HIGHCONTRAST highContrast = {};
	highContrast.cbSize = sizeof(highContrast);
	BOOL res = SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(highContrast), &highContrast, 0);

	if (!res)
	{
		DCHECK(false);
		return false;
	}

	return WI_IsFlagSet(highContrast.dwFlags, HCF_HIGHCONTRASTON);
}

HBRUSH DarkModeManager::GetBackgroundBrush() const
{
	return m_backgroundBrush.get();
}
