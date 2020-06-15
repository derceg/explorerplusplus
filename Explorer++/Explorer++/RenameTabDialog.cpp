// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RenameTabDialog.h"
#include "MainResource.h"
#include "TabContainer.h"
#include "../Helper/Macros.h"

const TCHAR RenameTabDialogPersistentSettings::SETTINGS_KEY[] = _T("RenameTab");

RenameTabDialog::RenameTabDialog(
	HINSTANCE hInstance, HWND hParent, int tabId, TabContainer *tabContainer) :
	DarkModeDialogBase(hInstance, IDD_RENAMETAB, hParent, false),
	m_tabId(tabId),
	m_tabContainer(tabContainer)
{
	m_prtdps = &RenameTabDialogPersistentSettings::GetInstance();

	m_connections.push_back(m_tabContainer->tabRemovedSignal.AddObserver(
		boost::bind(&RenameTabDialog::OnTabClosed, this, _1)));
}

INT_PTR RenameTabDialog::OnInitDialog()
{
	HWND hEditName = GetDlgItem(m_hDlg, IDC_RENAMETAB_NEWTABNAME);

	Tab *tab = m_tabContainer->GetTabOptional(m_tabId);

	SetWindowText(hEditName, tab->GetName().c_str());

	/* When this dialog is opened, the 'custom name' option will
	be selected by default (whether or not that is the actual
	option that is currently in effect). */
	CheckDlgButton(m_hDlg, IDC_RENAMETAB_USECUSTOMNAME, BST_CHECKED);

	EnableWindow(hEditName, TRUE);
	SendMessage(hEditName, EM_SETSEL, 0, -1);
	SetFocus(hEditName);

	AllowDarkModeForRadioButtons({ IDC_RENAMETAB_USEFOLDERNAME, IDC_RENAMETAB_USECUSTOMNAME });

	m_prtdps->RestoreDialogPosition(m_hDlg, false);

	return 0;
}

INT_PTR RenameTabDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam))
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

INT_PTR RenameTabDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
	return 0;
}

void RenameTabDialog::OnUseFolderName()
{
	HWND hEditName = GetDlgItem(m_hDlg, IDC_RENAMETAB_NEWTABNAME);

	EnableWindow(hEditName, FALSE);
}

void RenameTabDialog::OnUseCustomName()
{
	HWND hEditName = GetDlgItem(m_hDlg, IDC_RENAMETAB_NEWTABNAME);

	EnableWindow(hEditName, TRUE);
	SetFocus(hEditName);
}

void RenameTabDialog::OnOk()
{
	TCHAR szTabText[MAX_PATH];

	UINT uCheckStatus = IsDlgButtonChecked(m_hDlg, IDC_RENAMETAB_USEFOLDERNAME);

	Tab *tab = m_tabContainer->GetTabOptional(m_tabId);

	if (uCheckStatus == BST_CHECKED)
	{
		tab->ClearCustomName();
	}
	else
	{
		HWND hEditName = GetDlgItem(m_hDlg, IDC_RENAMETAB_NEWTABNAME);

		GetWindowText(hEditName, szTabText, SIZEOF_ARRAY(szTabText));

		if (lstrlen(szTabText) > 0)
		{
			tab->SetCustomName(szTabText);
		}
	}

	EndDialog(m_hDlg, 1);
}

void RenameTabDialog::OnCancel()
{
	EndDialog(m_hDlg, 0);
}

void RenameTabDialog::OnTabClosed(int tabId)
{
	if (tabId == m_tabId)
	{
		// Although tabs can't be closed from the user interface while
		// this dialog is open, plugins can close tabs at any time,
		// meaning the tab this dialog is renaming could be closed while
		// the dialog is open.
		EndDialog(m_hDlg, 0);
	}
}

void RenameTabDialog::SaveState()
{
	m_prtdps->SaveDialogPosition(m_hDlg);

	m_prtdps->m_bStateSaved = TRUE;
}

RenameTabDialogPersistentSettings::RenameTabDialogPersistentSettings() :
	DialogSettings(SETTINGS_KEY)
{
}

RenameTabDialogPersistentSettings &RenameTabDialogPersistentSettings::GetInstance()
{
	static RenameTabDialogPersistentSettings sfadps;
	return sfadps;
}