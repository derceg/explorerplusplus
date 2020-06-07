// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ApplicationToolbarButtonDialog.h"
#include "MainResource.h"
#include "ResourceHelper.h"

ApplicationToolbarButtonDialog::ApplicationToolbarButtonDialog(
	HINSTANCE hInstance, HWND hParent, ApplicationButton *Button, bool IsNew) :
	BaseDialog(hInstance, IDD_EDITAPPLICATIONBUTTON, hParent, false),
	m_Button(Button),
	m_IsNew(IsNew)
{
}

INT_PTR ApplicationToolbarButtonDialog::OnInitDialog()
{
	if (m_IsNew)
	{
		std::wstring newText =
			ResourceHelper::LoadString(GetInstance(), IDS_GENERAL_NEWAPPLICATIONBUTTON);
		SetWindowText(m_hDlg, newText.c_str());
	}

	AddTooltipForControl(IDC_APP_EDIT_COMMAND, IDC_APP_EDIT_COMMAND_TT);

	SetDlgItemText(m_hDlg, IDC_APP_EDIT_NAME, m_Button->Name.c_str());
	SetDlgItemText(m_hDlg, IDC_APP_EDIT_COMMAND, m_Button->Command.c_str());

	if (m_Button->Name.length() == 0 || m_Button->Command.length() == 0)
	{
		EnableWindow(GetDlgItem(m_hDlg, IDOK), FALSE);
	}

	UINT uCheck = m_Button->ShowNameOnToolbar ? BST_CHECKED : BST_UNCHECKED;
	CheckDlgButton(m_hDlg, IDC_CHECK_SHOWAPPNAME, uCheck);

	HWND hEditName = GetDlgItem(m_hDlg, IDC_APP_EDIT_NAME);
	SendMessage(hEditName, EM_SETSEL, 0, -1);
	SetFocus(hEditName);

	return 0;
}

INT_PTR ApplicationToolbarButtonDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (HIWORD(wParam) != 0)
	{
		switch (HIWORD(wParam))
		{
		case EN_CHANGE:
			if (LOWORD(wParam) != IDC_APP_EDIT_NAME && LOWORD(wParam) != IDC_APP_EDIT_COMMAND)
			{
				break;
			}

			BOOL enable = (GetWindowTextLength(GetDlgItem(m_hDlg, IDC_APP_EDIT_NAME)) != 0
				&& GetWindowTextLength(GetDlgItem(m_hDlg, IDC_APP_EDIT_COMMAND)) != 0);
			EnableWindow(GetDlgItem(m_hDlg, IDOK), enable);
			break;
		}
	}
	else
	{
		switch (LOWORD(wParam))
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
	}

	return 0;
}

void ApplicationToolbarButtonDialog::OnChooseFile()
{
	/* TODO: Text needs to be localized. */
	const TCHAR *filter = _T("Programs (*.exe)\0*.exe\0All Files\0*.*\0\0");
	TCHAR fullFileName[MAX_PATH] = EMPTY_STRING;

	OPENFILENAME ofn;
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = m_hDlg;
	ofn.lpstrFilter = filter;
	ofn.lpstrCustomFilter = nullptr;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = fullFileName;
	ofn.nMaxFile = SIZEOF_ARRAY(fullFileName);
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = nullptr;
	ofn.lpstrTitle = nullptr;
	ofn.Flags = OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt = _T("exe");
	ofn.lCustData = NULL;
	ofn.lpfnHook = nullptr;
	ofn.pvReserved = nullptr;
	ofn.dwReserved = 0;
	ofn.FlagsEx = 0;

	BOOL bRet = GetOpenFileName(&ofn);

	if (bRet)
	{
		std::wstring finalFileName = fullFileName;

		if (finalFileName.find(' ') != std::wstring::npos)
		{
			finalFileName = L"\"" + finalFileName + L"\"";
		}

		SetDlgItemText(m_hDlg, IDC_APP_EDIT_COMMAND, finalFileName.c_str());
	}
}

void ApplicationToolbarButtonDialog::OnOk()
{
	TCHAR name[512];
	GetDlgItemText(m_hDlg, IDC_APP_EDIT_NAME, name, SIZEOF_ARRAY(name));

	TCHAR command[512];
	GetDlgItemText(m_hDlg, IDC_APP_EDIT_COMMAND, command, SIZEOF_ARRAY(command));

	bool validated = true;

	if (lstrlen(name) == 0 || lstrlen(command) == 0)
	{
		validated = false;
	}

	if (!validated)
	{
		EndDialog(m_hDlg, 0);
		return;
	}

	m_Button->Name = name;
	m_Button->Command = command;
	m_Button->ShowNameOnToolbar = IsDlgButtonChecked(m_hDlg, IDC_CHECK_SHOWAPPNAME) == BST_CHECKED;

	EndDialog(m_hDlg, 1);
}

void ApplicationToolbarButtonDialog::OnCancel()
{
	EndDialog(m_hDlg, 0);
}

INT_PTR ApplicationToolbarButtonDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
	return 0;
}