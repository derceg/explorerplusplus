// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AboutDialog.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ThirdPartyCreditsDialog.h"
#include "Version.h"
#include "VersionHelper.h"
#include "../Helper/WindowHelper.h"
#include <fmt/format.h>
#include <fmt/xchar.h>

// Enable C4062: enumerator 'identifier' in switch of enum 'enumeration' is not handled
#pragma warning(default : 4062)

AboutDialog::AboutDialog(const ResourceLoader *resourceLoader, HINSTANCE resourceInstance,
	HWND hParent, ThemeManager *themeManager) :
	ThemedDialog(resourceLoader, resourceInstance, IDD_ABOUT, hParent, DialogSizingType::None,
		themeManager)
{
}

INT_PTR AboutDialog::OnInitDialog()
{
	m_icon.reset(reinterpret_cast<HICON>(LoadImage(GetModuleHandle(nullptr),
		MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, 32, 32, LR_VGACOLOR)));

	SendMessage(m_hDlg, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(m_icon.get()));

	m_mainIcon.reset(static_cast<HICON>(LoadImage(GetModuleHandle(nullptr),
		MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, 48, 48, LR_DEFAULTCOLOR)));
	SendDlgItemMessage(m_hDlg, IDC_ABOUT_STATIC_IMAGE, STM_SETICON,
		reinterpret_cast<WPARAM>(m_mainIcon.get()), 0);

	std::wstring versionTemplate =
		ResourceHelper::LoadString(GetResourceInstance(), IDS_ABOUT_VERSION);
	std::wstring platform;

	// Indicate which platform we are building for in the version string.
	switch (VersionHelper::GetPlatform())
	{
	case VersionHelper::Platform::x86:
		platform = ResourceHelper::LoadString(GetResourceInstance(), IDS_ABOUT_32BIT_BUILD);
		break;

	case VersionHelper::Platform::x64:
		platform = ResourceHelper::LoadString(GetResourceInstance(), IDS_ABOUT_64BIT_BUILD);
		break;

	case VersionHelper::Platform::Arm64:
		platform = ResourceHelper::LoadString(GetResourceInstance(), IDS_ABOUT_ARM64_BUILD);
		break;
	}

	std::wstring versionAndReleaseMode = VersionHelper::GetVersion().GetString();
	std::wstring releaseMode;

	switch (VersionHelper::GetChannel())
	{
	case VersionHelper::Channel::Stable:
		// There is no release mode shown when building a stable release.
		break;

	case VersionHelper::Channel::Beta:
		releaseMode = ResourceHelper::LoadString(GetResourceInstance(), IDS_RELEASE_MODE_BETA);
		break;

	case VersionHelper::Channel::Dev:
		releaseMode = ResourceHelper::LoadString(GetResourceInstance(), IDS_RELEASE_MODE_DEV);
		break;
	}

	if (!releaseMode.empty())
	{
		versionAndReleaseMode += L" " + releaseMode;
	}

	std::wstring version = fmt::format(fmt::runtime(versionTemplate),
		fmt::arg(L"version_string", versionAndReleaseMode), fmt::arg(L"platform", platform));

	std::wstring buildDateTemplate =
		ResourceHelper::LoadString(GetResourceInstance(), IDS_ABOUT_BUILD_DATE);
	std::wstring buildDate = fmt::format(fmt::runtime(buildDateTemplate),
		fmt::arg(L"build_date", VersionHelper::GetBuildDate()));

	std::wstring versionInfo = version + L"\r\n\r\n" + buildDate;
	SetDlgItemText(m_hDlg, IDC_VERSION_INFORMATION, versionInfo.c_str());

	CenterWindow(GetParent(m_hDlg), m_hDlg);

	return TRUE;
}

INT_PTR AboutDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam))
	{
	case IDOK:
		EndDialog(m_hDlg, 1);
		break;

	case IDCANCEL:
		EndDialog(m_hDlg, 0);
		break;
	}

	return 0;
}

INT_PTR AboutDialog::OnNotify(NMHDR *pnmhdr)
{
	switch (pnmhdr->code)
	{
	case NM_CLICK:
	case NM_RETURN:
	{
		if (pnmhdr->idFrom == IDC_SITELINK)
		{
			auto pnmlink = reinterpret_cast<PNMLINK>(pnmhdr);

			ShellExecute(nullptr, L"open", pnmlink->item.szUrl, nullptr, nullptr, SW_SHOWNORMAL);
		}
		else if (pnmhdr->idFrom == IDC_THIRD_PARTY_CREDITS_LINK)
		{
			ThirdPartyCreditsDialog thirdPartyCreditsDialog(m_resourceLoader, GetResourceInstance(),
				m_hDlg, GetThemeManager());
			thirdPartyCreditsDialog.ShowModalDialog();
		}
	}
	break;
	}

	return 0;
}

INT_PTR AboutDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
	return 0;
}
