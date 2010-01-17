/******************************************************************
 *
 * Project: Explorer++
 * File: RenameTabDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the 'Rename Tab' dialog.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"

BOOL	g_EditNameEnabled;
int		g_iTab;

INT_PTR CALLBACK RenameTabProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static CContainer *pContainer;

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			RenameTabInfo_t *prti;

			prti = (RenameTabInfo_t *)lParam;

			pContainer = (CContainer *)prti->pContainer;
			g_iTab = prti->iTab;
		}
		break;
	}

	return pContainer->RenameTabProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK CContainer::RenameTabProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			OnRenameTabInit(hDlg);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
			case IDC_RENAMETAB_USEFOLDERNAME:
				{
					HWND	hEditName;

					hEditName = GetDlgItem(hDlg,IDC_RENAMETAB_NEWTABNAME);

					EnableWindow(hEditName,FALSE);
				}
				break;

			case IDC_RENAMETAB_USECUSTOMNAME:
				{
					HWND	hEditName;

					hEditName = GetDlgItem(hDlg,IDC_RENAMETAB_NEWTABNAME);

					EnableWindow(hEditName,TRUE);

					SetFocus(hEditName);
				}
				break;

			case IDOK:
				OnRenameTabOk(hDlg);
				break;

			case IDCANCEL:
				EndDialog(hDlg,0);
				break;
			}
			break;

		case WM_CLOSE:
			EndDialog(hDlg,0);
			break;
	}

	return 0;
}

void CContainer::OnRenameTabInit(HWND hDlg)
{
	HWND	hEditName;
	TCITEM	tcItem;

	hEditName = GetDlgItem(hDlg,IDC_RENAMETAB_NEWTABNAME);

	tcItem.mask			= TCIF_PARAM;
	TabCtrl_GetItem(m_hTabCtrl,g_iTab,&tcItem);

	SetWindowText(hEditName,m_TabInfo[(int)tcItem.lParam].szName);

	g_EditNameEnabled = m_TabInfo[(int)tcItem.lParam].bUseCustomName;

	/* When this dialog is opened, the 'custom name' option will
	be selected by default (whether or not that is the actual
	option that is currently in effect). */
	CheckDlgButton(hDlg,IDC_RENAMETAB_USECUSTOMNAME,BST_CHECKED);

	EnableWindow(hEditName,TRUE);

	/* Select all the text in the name edit control. */
	SendMessage(hEditName,EM_SETSEL,0,-1);

	/* Focus on the name edit control. */
	SetFocus(hEditName);
}

void CContainer::OnRenameTabOk(HWND hDlg)
{
	HWND	hEditName;
	TCITEM	tcItem;
	UINT	uCheckStatus;
	TCHAR	szTabText[MAX_PATH];

	tcItem.mask			= TCIF_PARAM;
	TabCtrl_GetItem(m_hTabCtrl,g_iTab,&tcItem);

	uCheckStatus = IsDlgButtonChecked(hDlg,IDC_RENAMETAB_USEFOLDERNAME);

	/* If the button is checked, use the
	current folders name as the tab text.
	If the button is not checked, use
	whatever text is in the edit control. */
	if(uCheckStatus == BST_CHECKED)
	{
		LPITEMIDLIST	pidlDirectory = NULL;

		pidlDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();
		GetDisplayName(pidlDirectory,szTabText,SHGDN_INFOLDER);

		CoTaskMemFree(pidlDirectory);
	}
	else
	{
		hEditName = GetDlgItem(hDlg,IDC_RENAMETAB_NEWTABNAME);

		GetWindowText(hEditName,szTabText,SIZEOF_ARRAY(szTabText));
	}

	StringCchCopy(m_TabInfo[(int)tcItem.lParam].szName,
		SIZEOF_ARRAY(m_TabInfo[(int)tcItem.lParam].szName),szTabText);
	m_TabInfo[(int)tcItem.lParam].bUseCustomName = (uCheckStatus != BST_CHECKED);

	tcItem.mask		= TCIF_TEXT;
	tcItem.pszText	= szTabText;
	TabCtrl_SetItem(m_hTabCtrl,g_iTab,&tcItem);

	EndDialog(hDlg,1);
}