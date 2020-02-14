// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AboutDialog.h"
#include "MainResource.h"
#include "Version.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/Macros.h"
#include "../Helper/WindowHelper.h"

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

	TCHAR szVersion[64];
	TCHAR szBuild[64];
	TCHAR szBuildDate[64];
	TCHAR szTemp[64];

	/* Indicate which architecture (32-bit or
	64-bit) we are building for in the version
	string.*/
#ifdef WIN64
	LoadString(GetInstance(),IDS_ABOUT_64BIT_BUILD,
		szBuild,SIZEOF_ARRAY(szBuild));
#else
	LoadString(GetInstance(),IDS_ABOUT_32BIT_BUILD,
		szBuild,SIZEOF_ARRAY(szBuild));
#endif

	LoadString(GetInstance(),IDS_ABOUT_UNICODE_BUILD,
		szTemp,SIZEOF_ARRAY(szTemp));
	StringCchCat(szBuild,SIZEOF_ARRAY(szBuild),_T(" "));
	StringCchCat(szBuild,SIZEOF_ARRAY(szBuild),szTemp);

	GetDlgItemText(m_hDlg,IDC_STATIC_VERSIONNUMBER,szTemp,SIZEOF_ARRAY(szTemp));
	StringCchPrintf(szVersion,SIZEOF_ARRAY(szVersion),szTemp,VERSION_STRING_W,szBuild);
	SetDlgItemText(m_hDlg,IDC_STATIC_VERSIONNUMBER,szVersion);

	GetDlgItemText(m_hDlg,IDC_STATIC_BUILDDATE,szTemp,SIZEOF_ARRAY(szTemp));
	StringCchPrintf(szBuildDate,SIZEOF_ARRAY(szBuildDate),szTemp,BUILD_DATE_STRING);

	SetDlgItemText(m_hDlg,IDC_STATIC_BUILDDATE,szBuildDate);

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