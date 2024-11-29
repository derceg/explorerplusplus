// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ApplicationEditorDialog.h"
#include "ApplicationModel.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/Controls.h"
#include "../Helper/Macros.h"
#include "../Helper/WindowHelper.h"

namespace Applications
{

ApplicationEditorDialog::ApplicationEditorDialog(HWND parent, HINSTANCE resourceInstance,
	ThemeManager *themeManager, ApplicationModel *model, std::unique_ptr<EditDetails> editDetails) :
	ThemedDialog(resourceInstance, IDD_EDITAPPLICATIONBUTTON, parent, DialogSizingType::None,
		themeManager),
	m_model(model),
	m_editDetails(std::move(editDetails))
{
}

INT_PTR ApplicationEditorDialog::OnInitDialog()
{
	if (m_editDetails->type == EditDetails::Type::NewItem)
	{
		std::wstring newText =
			ResourceHelper::LoadString(GetResourceInstance(), IDS_GENERAL_NEWAPPLICATIONBUTTON);
		SetWindowText(m_hDlg, newText.c_str());
	}

	AddTooltipForControl(m_tipWnd, GetDlgItem(m_hDlg, IDC_APP_EDIT_COMMAND), GetResourceInstance(),
		IDS_APP_EDIT_COMMAND_TOOLTIP);

	const Application *targetApplication = m_editDetails->type == EditDetails::Type::NewItem
		? m_editDetails->newApplication.get()
		: m_editDetails->existingApplication;

	std::wstring name = targetApplication->GetName();
	std::wstring command = targetApplication->GetCommand();
	bool showNameOnToolbar = targetApplication->GetShowNameOnToolbar();

	SetDlgItemText(m_hDlg, IDC_APP_EDIT_NAME, name.c_str());
	SetDlgItemText(m_hDlg, IDC_APP_EDIT_COMMAND, command.c_str());

	if (name.length() == 0 || command.length() == 0)
	{
		EnableWindow(GetDlgItem(m_hDlg, IDOK), FALSE);
	}

	UINT uCheck = showNameOnToolbar ? BST_CHECKED : BST_UNCHECKED;
	CheckDlgButton(m_hDlg, IDC_CHECK_SHOWAPPNAME, uCheck);

	HWND hEditName = GetDlgItem(m_hDlg, IDC_APP_EDIT_NAME);
	SendMessage(hEditName, EM_SETSEL, 0, -1);
	SetFocus(hEditName);

	return 0;
}

INT_PTR ApplicationEditorDialog::OnCommand(WPARAM wParam, LPARAM lParam)
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

void ApplicationEditorDialog::OnChooseFile()
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

void ApplicationEditorDialog::OnOk()
{
	std::wstring name = GetDlgItemString(m_hDlg, IDC_APP_EDIT_NAME);
	std::wstring command = GetDlgItemString(m_hDlg, IDC_APP_EDIT_COMMAND);

	if (name.empty() || command.empty())
	{
		EndDialog(m_hDlg, 0);
		return;
	}

	bool showNameOnToolbar = IsDlgButtonChecked(m_hDlg, IDC_CHECK_SHOWAPPNAME) == BST_CHECKED;

	ApplyEdits(name, command, showNameOnToolbar);

	EndDialog(m_hDlg, 1);
}

void ApplicationEditorDialog::ApplyEdits(const std::wstring &newName,
	const std::wstring &newCommand, bool newShowNameOnToolbar)
{
	Application *targetApplication = m_editDetails->type == EditDetails::Type::NewItem
		? m_editDetails->newApplication.get()
		: m_editDetails->existingApplication;

	targetApplication->SetName(newName);
	targetApplication->SetCommand(newCommand);
	targetApplication->SetShowNameOnToolbar(newShowNameOnToolbar);

	if (m_editDetails->type == EditDetails::Type::NewItem)
	{
		size_t index;
		size_t numItems = m_model->GetItems().size();

		if (m_editDetails->index.has_value() && m_editDetails->index.value() <= numItems)
		{
			index = m_editDetails->index.value();
		}
		else
		{
			index = numItems;
		}

		m_model->AddItem(std::move(m_editDetails->newApplication), index);
	}
}

void ApplicationEditorDialog::OnCancel()
{
	EndDialog(m_hDlg, 0);
}

INT_PTR ApplicationEditorDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
	return 0;
}

}
