// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AboutDialog.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "Version.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/Macros.h"
#include "../Helper/WindowHelper.h"
#include <boost/format.hpp>

AboutDialog::AboutDialog(HINSTANCE hInstance, HWND hParent) :
	BaseDialog(hInstance, IDD_ABOUT, hParent, false)
{
	
}

INT_PTR AboutDialog::OnInitDialog()
{
	m_icon.reset(reinterpret_cast<HICON>(LoadImage(GetModuleHandle(nullptr),
		MAKEINTRESOURCE(IDI_MAIN),IMAGE_ICON,
		32,32,LR_VGACOLOR)));

	SendMessage(m_hDlg,WM_SETICON,ICON_SMALL,reinterpret_cast<LPARAM>(m_icon.get()));

	m_mainIcon.reset(static_cast<HICON>(LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, 48, 48, LR_DEFAULTCOLOR)));
	SendDlgItemMessage(m_hDlg, IDC_ABOUT_STATIC_IMAGE, STM_SETICON, reinterpret_cast<WPARAM>(m_mainIcon.get()), 0);

	std::wstring versionTemplate = ResourceHelper::LoadString(GetInstance(), IDS_ABOUT_VERSION);
	std::wstring platform;

	// Indicate which platform (32-bit or 64-bit) we are building for in the version string.
#ifdef WIN64
	platform = ResourceHelper::LoadString(GetInstance(), IDS_ABOUT_64BIT_BUILD);
#else
	platform = ResourceHelper::LoadString(GetInstance(), IDS_ABOUT_32BIT_BUILD);
#endif

	std::wstring version = (boost::wformat(versionTemplate) % VERSION_STRING_W % platform).str();

	std::wstring buildDateTemplate = ResourceHelper::LoadString(GetInstance(), IDS_ABOUT_BUILD_DATE);
	std::wstring buildDate = (boost::wformat(buildDateTemplate) % BUILD_DATE_STRING).str();

	std::wstring versionInfo = version + L"\r\n\r\n" + buildDate;
	SetDlgItemText(m_hDlg, IDC_VERSION_INFORMATION, versionInfo.c_str());

	CenterWindow(GetParent(m_hDlg),m_hDlg);

	return TRUE;
}

INT_PTR AboutDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch(LOWORD(wParam))
	{
	case IDOK:
		EndDialog(m_hDlg,1);
		break;

	case IDCANCEL:
		EndDialog(m_hDlg,0);
		break;
	}

	return 0;
}

INT_PTR AboutDialog::OnNotify(NMHDR *pnmhdr)
{
	switch(pnmhdr->code)
	{
	case NM_CLICK:
	case NM_RETURN:
		{
			if(pnmhdr->hwndFrom == GetDlgItem(m_hDlg,IDC_SITELINK))
			{
				PNMLINK pnmlink = reinterpret_cast<PNMLINK>(pnmhdr);

				ShellExecute(nullptr,L"open",pnmlink->item.szUrl,
					nullptr, nullptr,SW_SHOW);
			}
		}
		break;
	}

	return 0;
}

INT_PTR AboutDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}