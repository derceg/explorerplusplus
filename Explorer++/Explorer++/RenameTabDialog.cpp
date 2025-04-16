// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RenameTabDialog.h"
#include "App.h"
#include "MainResource.h"
#include "../Helper/WindowHelper.h"

const TCHAR RenameTabDialogPersistentSettings::SETTINGS_KEY[] = _T("RenameTab");

RenameTabDialog::RenameTabDialog(HWND parent, App *app, Tab *tab) :
	BaseDialog(app->GetResourceLoader(), IDD_RENAMETAB, parent, DialogSizingType::None),
	m_tab(tab)
{
	m_prtdps = &RenameTabDialogPersistentSettings::GetInstance();

	m_connections.push_back(app->GetTabEvents()->AddRemovedObserver(
		std::bind_front(&RenameTabDialog::OnTabClosed, this),
		TabEventScope::ForBrowser(*m_tab->GetBrowser())));
}

INT_PTR RenameTabDialog::OnInitDialog()
{
	HWND hEditName = GetDlgItem(m_hDlg, IDC_RENAMETAB_NEWTABNAME);

	SetWindowText(hEditName, m_tab->GetName().c_str());

	/* When this dialog is opened, the 'custom name' option will
	be selected by default (whether or not that is the actual
	option that is currently in effect). */
	CheckDlgButton(m_hDlg, IDC_RENAMETAB_USECUSTOMNAME, BST_CHECKED);

	EnableWindow(hEditName, TRUE);
	SendMessage(hEditName, EM_SETSEL, 0, -1);
	SetFocus(hEditName);

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
	UINT uCheckStatus = IsDlgButtonChecked(m_hDlg, IDC_RENAMETAB_USEFOLDERNAME);

	if (uCheckStatus == BST_CHECKED)
	{
		m_tab->ClearCustomName();
	}
	else
	{
		HWND hEditName = GetDlgItem(m_hDlg, IDC_RENAMETAB_NEWTABNAME);

		std::wstring tabText = GetWindowString(hEditName);
		m_tab->SetCustomName(tabText);
	}

	EndDialog(m_hDlg, 1);
}

void RenameTabDialog::OnCancel()
{
	EndDialog(m_hDlg, 0);
}

void RenameTabDialog::OnTabClosed(const Tab &tab)
{
	if (&tab == m_tab)
	{
		// Although tabs can't be closed from the user interface while this dialog is open, plugins
		// can close tabs at any time, meaning the tab this dialog is renaming could be closed while
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
