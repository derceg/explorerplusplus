// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AddBookmarkFolderDialog.h"
#include "MainResource.h"

const TCHAR AddBookmarkFolderDialogPersistentSettings::SETTINGS_KEY[] = _T("AddBookmarkFolder");

AddBookmarkFolderDialog::AddBookmarkFolderDialog(HINSTANCE hInstance,
	int iResource,HWND hParent) :
CBaseDialog(hInstance,iResource,hParent,true)
{
	m_pnbfdps = &AddBookmarkFolderDialogPersistentSettings::GetInstance();
}

INT_PTR AddBookmarkFolderDialog::OnInitDialog()
{
	return 0;
}

INT_PTR AddBookmarkFolderDialog::OnCommand(WPARAM wParam,LPARAM lParam)
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

void AddBookmarkFolderDialog::OnOk()
{
	EndDialog(m_hDlg,1);
}

void AddBookmarkFolderDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

INT_PTR AddBookmarkFolderDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

AddBookmarkFolderDialogPersistentSettings::AddBookmarkFolderDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{

}

AddBookmarkFolderDialogPersistentSettings& AddBookmarkFolderDialogPersistentSettings::GetInstance()
{
	static AddBookmarkFolderDialogPersistentSettings nbfdps;
	return nbfdps;
}