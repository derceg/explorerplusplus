// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "NewBookmarkFolderDialog.h"
#include "MainResource.h"


const TCHAR CNewBookmarkFolderDialogPersistentSettings::SETTINGS_KEY[] = _T("NewBookmarkFolder");

CNewBookmarkFolderDialog::CNewBookmarkFolderDialog(HINSTANCE hInstance,
	int iResource,HWND hParent) :
CBaseDialog(hInstance,iResource,hParent,true)
{
	m_pnbfdps = &CNewBookmarkFolderDialogPersistentSettings::GetInstance();
}

INT_PTR CNewBookmarkFolderDialog::OnInitDialog()
{
	return 0;
}

INT_PTR CNewBookmarkFolderDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch(LOWORD(wParam))
	{
	case IDOK:
		OnOk();
		break;

	case IDCANCEL:
		OnCancel();
		break;
	}

	return 0;
}

void CNewBookmarkFolderDialog::OnOk()
{
	EndDialog(m_hDlg,1);
}

void CNewBookmarkFolderDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

INT_PTR CNewBookmarkFolderDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

CNewBookmarkFolderDialogPersistentSettings::CNewBookmarkFolderDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{

}

CNewBookmarkFolderDialogPersistentSettings& CNewBookmarkFolderDialogPersistentSettings::GetInstance()
{
	static CNewBookmarkFolderDialogPersistentSettings nbfdps;
	return nbfdps;
}