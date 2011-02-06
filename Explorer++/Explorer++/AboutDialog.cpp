/******************************************************************
 *
 * Project: Explorer++
 * File: AboutDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Manages the 'About' dialog box.
 *
 * Notes:
 *  - Always center dialog. Position is not saved/loaded.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "Explorer++_internal.h"
#include "AboutDialog.h"
#include "MainResource.h"
#include "../Helper/Helper.h"
#include "../Helper/BaseDialog.h"

#ifndef _DEBUG
	#include "Version.h"
#endif


CAboutDialog::CAboutDialog(HINSTANCE hInstance,int iResource,HWND hParent) :
CBaseDialog(hInstance,iResource,hParent,false)
{
	
}

CAboutDialog::~CAboutDialog()
{

}

BOOL CAboutDialog::OnInitDialog()
{
	TCHAR	szVersion[64];
	TCHAR	szTemp[64];
	TCHAR	szBuild[64];
	TCHAR	szBuildDate[64];

	m_hIcon = (HICON)LoadImage(GetModuleHandle(0),
		MAKEINTRESOURCE(IDI_MAIN),IMAGE_ICON,
		32,32,LR_VGACOLOR);

	SendMessage(m_hDlg,WM_SETICON,ICON_SMALL,(LPARAM)m_hIcon);

	/* Indicate which architecture (32-bit or
	64-bit) we are building for in the version
	string.*/
#ifdef WIN64
	StringCchCopy(szBuild,SIZEOF_ARRAY(szBuild),_T("64-bit"));
#else
	StringCchCopy(szBuild,SIZEOF_ARRAY(szBuild),_T("32-bit"));
#endif

#ifdef UNICODE
	StringCchCat(szBuild,SIZEOF_ARRAY(szBuild),_T(" Unicode build"));
#else
	StringCchCat(szBuild,SIZEOF_ARRAY(szBuild),_T(" build"));
#endif

	GetDlgItemText(m_hDlg,IDC_STATIC_VERSIONNUMBER,szTemp,SIZEOF_ARRAY(szTemp));
	StringCchPrintf(szVersion,SIZEOF_ARRAY(szVersion),szTemp,NExplorerplusplus::VERSION_NUMBER,szBuild);

	SetDlgItemText(m_hDlg,IDC_STATIC_VERSIONNUMBER,szVersion);

	/* We'll only show a build date in non-debug mode. */
#ifndef _DEBUG
	GetDlgItemText(m_hDlg,IDC_STATIC_BUILDDATE,szTemp,SIZEOF_ARRAY(szTemp));
	StringCchPrintf(szBuildDate,SIZEOF_ARRAY(szBuildDate),szTemp,VERSION_BUILD_DATE);
#else
	StringCchCopy(szBuildDate,SIZEOF_ARRAY(szBuildDate),_T("[Debug Build]"));
#endif

	SetDlgItemText(m_hDlg,IDC_STATIC_BUILDDATE,szBuildDate);

	CenterWindow(GetParent(m_hDlg),m_hDlg);

	return TRUE;
}

BOOL CAboutDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
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

BOOL CAboutDialog::OnNotify(NMHDR *pnmhdr)
{
	switch(pnmhdr->code)
	{
	case NM_CLICK:
	case NM_RETURN:
		{
			if(pnmhdr->hwndFrom == GetDlgItem(m_hDlg,IDC_SITELINK))
			{
				PNMLINK pnmlink = reinterpret_cast<PNMLINK>(pnmhdr);

				ShellExecute(NULL,L"open",pnmlink->item.szUrl,
					NULL,NULL,SW_SHOW);
			}
		}
		break;
	}

	return 0;
}

BOOL CAboutDialog::OnClose()
{
	DestroyIcon(m_hIcon);

	EndDialog(m_hDlg,0);
	return 0;
}