/******************************************************************
 *
 * Project: Explorer++
 * File: iServiceProvider.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * New/edit dialog for the application toolbar.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "ApplicationToolbarButtonDialog.h"
#include "MainResource.h"


CApplicationToolbarButtonDialog::CApplicationToolbarButtonDialog(HINSTANCE hInstance,
	int iResource,HWND hParent,ApplicationButton_t *Button,bool IsNew) :
m_Button(Button),
m_IsNew(IsNew),
CBaseDialog(hInstance,iResource,hParent,false)
{
	
}

CApplicationToolbarButtonDialog::~CApplicationToolbarButtonDialog()
{

}

INT_PTR CApplicationToolbarButtonDialog::OnInitDialog()
{
	if(m_IsNew)
	{
		TCHAR szTemp[64];
		LoadString(GetInstance(),IDS_GENERAL_NEWAPPLICATIONBUTTON,
			szTemp,SIZEOF_ARRAY(szTemp));
		SetWindowText(m_hDlg,szTemp);
	}

	SetDlgItemText(m_hDlg,IDC_APP_EDIT_NAME,m_Button->Name.c_str());
	SetDlgItemText(m_hDlg,IDC_APP_EDIT_COMMAND,m_Button->Command.c_str());

	UINT uCheck = m_Button->ShowNameOnToolbar ? BST_CHECKED : BST_UNCHECKED;
	CheckDlgButton(m_hDlg,IDC_CHECK_SHOWAPPNAME,uCheck);

	HWND hEditName = GetDlgItem(m_hDlg,IDC_APP_EDIT_NAME);
	SendMessage(hEditName,EM_SETSEL,0,-1);
	SetFocus(hEditName);

	return 0;
}

INT_PTR CApplicationToolbarButtonDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	switch(LOWORD(wParam))
	{
	case IDC_APP_BUTTON_CHOOSE_FILE:
		OnChooseFile();
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

void CApplicationToolbarButtonDialog::OnChooseFile()
{
	/* TODO: Text needs to be localized. */
	TCHAR *Filter = _T("Programs (*.exe)\0*.exe\0All Files\0*.*\0\0");
	TCHAR FullFileName[MAX_PATH] = EMPTY_STRING;

	OPENFILENAME ofn;
	ofn.lStructSize			= sizeof(ofn);
	ofn.hwndOwner			= m_hDlg;
	ofn.lpstrFilter			= Filter;
	ofn.lpstrCustomFilter	= NULL;
	ofn.nMaxCustFilter		= 0;
	ofn.nFilterIndex		= 0;
	ofn.lpstrFile			= FullFileName;
	ofn.nMaxFile			= SIZEOF_ARRAY(FullFileName);
	ofn.lpstrFileTitle		= NULL;
	ofn.nMaxFileTitle		= 0;
	ofn.lpstrInitialDir		= NULL;
	ofn.lpstrTitle			= NULL;
	ofn.Flags				= OFN_ENABLESIZING|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt			= _T("exe");
	ofn.lCustData			= NULL;
	ofn.lpfnHook			= NULL;
	ofn.pvReserved			= NULL;
	ofn.dwReserved			= NULL;
	ofn.FlagsEx				= NULL;

	BOOL bRet = GetOpenFileName(&ofn);

	if(bRet)
	{
		SetDlgItemText(m_hDlg,IDC_APP_EDIT_COMMAND,FullFileName);
	}
}

void CApplicationToolbarButtonDialog::OnOk()
{
	/* TODO: Must not be empty. */
	TCHAR Name[512];
	GetDlgItemText(m_hDlg,IDC_APP_EDIT_NAME,Name,SIZEOF_ARRAY(Name));

	TCHAR Command[512];
	GetDlgItemText(m_hDlg,IDC_APP_EDIT_COMMAND,Command,SIZEOF_ARRAY(Command));

	m_Button->Name = Name;
	m_Button->Command = Command;
	m_Button->ShowNameOnToolbar = IsDlgButtonChecked(m_hDlg,IDC_CHECK_SHOWAPPNAME) == BST_CHECKED;

	EndDialog(m_hDlg,1);
}

void CApplicationToolbarButtonDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

INT_PTR CApplicationToolbarButtonDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}