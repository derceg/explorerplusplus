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
	m_darkModeEnabled = m_ShouldAppsUseDarkMode() && !IsHighContrast();
}

void DarkModeHelper::EnableForApp()
{
	AllowDarkModeForApp(true);
	FlushMenuThemes();
	RefreshImmersiveColorPolicyState();
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

bool DarkModeHelper::IsDarkModeEnabled() const
{
	return m_darkModeEnabled;
}