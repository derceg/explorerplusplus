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
#include "Explorer++_internal.h"
#include "RenameTabDialog.h"
#include "MainResource.h"
#include "../Helper/Helper.h"
#include "../Helper/ShellHelper.h"


const TCHAR CRenameTabDialogPersistentSettings::SETTINGS_KEY[] = _T("RenameTab");

CRenameTabDialog::CRenameTabDialog(HINSTANCE hInstance,
	int iResource,HWND hParent,IExplorerplusplus *pexpp) :
CBaseDialog(hInstance,iResource,hParent,false)
{
	m_pexpp = pexpp;

	m_prtdps = &CRenameTabDialogPersistentSettings::GetInstance();

	/* TODO: Need tab name. Query from tab directly. */
	/*TCITEM tcItem;
	tcItem.mask			= TCIF_PARAM;
	TabCtrl_GetItem(m_hTabCtrl,g_iTab,&tcItem);

	SetWindowText(hEditName,m_TabInfo[(int)tcItem.lParam].szName);*/
}

CRenameTabDialog::~CRenameTabDialog()
{

}

BOOL CRenameTabDialog::OnInitDialog()
{
	HWND hEditName = GetDlgItem(m_hDlg,IDC_RENAMETAB_NEWTABNAME);

	/* TODO: */
	//SetWindowText(hEditName,m_szTabName);

	/* When this dialog is opened, the 'custom name' option will
	be selected by default (whether or not that is the actual
	option that is currently in effect). */
	CheckDlgButton(m_hDlg,IDC_RENAMETAB_USECUSTOMNAME,BST_CHECKED);

	EnableWindow(hEditName,TRUE);
	SendMessage(hEditName,EM_SETSEL,0,-1);
	SetFocus(hEditName);

	m_prtdps->RestoreDialogPosition(m_hDlg,false);

	return 0;
}

BOOL CRenameTabDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	switch(LOWORD(wParam))
	{
	case IDC_RENAMETAB_USEFOLDERNAME:
		OnUseFolderName();
		break;

	case IDC_RENAMETAB_USECUSTOMNAME:
		OnUseCustomName();
		break;

	case IDOK:
		OnOk();
		break;

	case IDCANCEL:
		OnCancel();
		break;
	}

	return 0;
}

BOOL CRenameTabDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

void CRenameTabDialog::OnUseFolderName()
{
	HWND hEditName = GetDlgItem(m_hDlg,IDC_RENAMETAB_NEWTABNAME);

	EnableWindow(hEditName,FALSE);
}

void CRenameTabDialog::OnUseCustomName()
{
	HWND hEditName = GetDlgItem(m_hDlg,IDC_RENAMETAB_NEWTABNAME);

	EnableWindow(hEditName,TRUE);
	SetFocus(hEditName);
}

void CRenameTabDialog::OnOk()
{
	TCHAR szTabText[MAX_PATH];

	UINT uCheckStatus = IsDlgButtonChecked(m_hDlg,IDC_RENAMETAB_USEFOLDERNAME);

	if(uCheckStatus == BST_CHECKED)
	{
		LPITEMIDLIST pidlDirectory = NULL;

		pidlDirectory = m_pexpp->GetActiveShellBrowser()->QueryCurrentDirectoryIdl();
		GetDisplayName(pidlDirectory,szTabText,SHGDN_INFOLDER);

		CoTaskMemFree(pidlDirectory);
	}
	else
	{
		HWND hEditName = GetDlgItem(m_hDlg,IDC_RENAMETAB_NEWTABNAME);

		GetWindowText(hEditName,szTabText,SIZEOF_ARRAY(szTabText));
	}

	/* TODO: Need to be able to change tab name. */

	EndDialog(m_hDlg,1);
}

void CRenameTabDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

void CRenameTabDialog::SaveState()
{
	m_prtdps->SaveDialogPosition(m_hDlg);

	m_prtdps->m_bStateSaved = TRUE;
}

CRenameTabDialogPersistentSettings::CRenameTabDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{

}

CRenameTabDialogPersistentSettings::~CRenameTabDialogPersistentSettings()
{
	
}

CRenameTabDialogPersistentSettings& CRenameTabDialogPersistentSettings::GetInstance()
{
	static CRenameTabDialogPersistentSettings sfadps;
	return sfadps;
}