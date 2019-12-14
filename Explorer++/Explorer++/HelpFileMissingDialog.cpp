// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "HelpFileMissingDialog.h"
#include "MainResource.h"
#include "../Helper/Helper.h"
#include "../Helper/WindowHelper.h"

CHelpFileMissingDialog::CHelpFileMissingDialog(HINSTANCE hInstance, HWND hParent) :
	CBaseDialog(hInstance, IDD_HELPFILEMISSING, hParent, false)
{
	
}

INT_PTR CHelpFileMissingDialog::OnInitDialog()
{
	CenterWindow(GetParent(m_hDlg),m_hDlg);

	return TRUE;
}

INT_PTR CHelpFileMissingDialog::OnCommand(WPARAM wParam,LPARAM lParam)
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

INT_PTR CHelpFileMissingDialog::OnNotify(NMHDR *pnmhdr)
{
	switch(pnmhdr->code)
	{
	case NM_CLICK:
	case NM_RETURN:
		{
			if(pnmhdr->hwndFrom == GetDlgItem(m_hDlg,IDC_SYSLINK_DOWNLOAD) ||
				pnmhdr->hwndFrom == GetDlgItem(m_hDlg, IDC_SYSLINK_READTHEDOCS))
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

INT_PTR CHelpFileMissingDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}