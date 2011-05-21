/******************************************************************
 *
 * Project: Explorer++
 * File: NewBookmarkFolderDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the 'New Folder' dialog (for bookmarks).
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "NewBookmarkFolderDialog.h"
#include "MainResource.h"
#include "../Helper/Bookmark.h"


const TCHAR CNewBookmarkFolderDialogPersistentSettings::SETTINGS_KEY[] = _T("NewBookmarkFolder");

CNewBookmarkFolderDialog::CNewBookmarkFolderDialog(HINSTANCE hInstance,
	int iResource,HWND hParent) :
CBaseDialog(hInstance,iResource,hParent,true)
{
	m_pnbfdps = &CNewBookmarkFolderDialogPersistentSettings::GetInstance();
}

CNewBookmarkFolderDialog::~CNewBookmarkFolderDialog()
{

}

BOOL CNewBookmarkFolderDialog::OnInitDialog()
{
	return 0;
}

BOOL CNewBookmarkFolderDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
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

BOOL CNewBookmarkFolderDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

CNewBookmarkFolderDialogPersistentSettings::CNewBookmarkFolderDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{

}

CNewBookmarkFolderDialogPersistentSettings::~CNewBookmarkFolderDialogPersistentSettings()
{
	
}

CNewBookmarkFolderDialogPersistentSettings& CNewBookmarkFolderDialogPersistentSettings::GetInstance()
{
	static CNewBookmarkFolderDialogPersistentSettings nbfdps;
	return nbfdps;
}