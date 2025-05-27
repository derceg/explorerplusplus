// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "WildcardSelectDialog.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "ShellBrowser/ShellBrowser.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/XMLSettings.h"

const TCHAR WildcardSelectDialogPersistentSettings::SETTINGS_KEY[] = _T("WildcardSelect");

const TCHAR WildcardSelectDialogPersistentSettings::SETTING_PATTERN_LIST[] = _T("Pattern");
const TCHAR WildcardSelectDialogPersistentSettings::SETTING_CURRENT_TEXT[] = _T("CurrentText");

WildcardSelectDialog::WildcardSelectDialog(const ResourceLoader *resourceLoader, HWND parent,
	ShellBrowser *shellBrowser, SelectionType selectionType) :
	BaseDialog(resourceLoader, IDD_WILDCARDSELECT, parent, DialogSizingType::Horizontal),
	m_shellBrowser(shellBrowser),
	m_selectionType(selectionType)
{
	m_pwsdps = &WildcardSelectDialogPersistentSettings::GetInstance();

	m_connections.push_back(m_shellBrowser->AddDestroyedObserver(
		std::bind_front(&WildcardSelectDialog::OnShellBrowserDestroyed, this)));
}

INT_PTR WildcardSelectDialog::OnInitDialog()
{
	m_icon.reset(LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_MAIN)));
	SetClassLongPtr(m_hDlg, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(m_icon.get()));

	HWND hComboBox = GetDlgItem(m_hDlg, IDC_SELECTGROUP_COMBOBOX);

	for (const auto &strPattern : m_pwsdps->m_PatternList)
	{
		ComboBox_InsertString(hComboBox, -1, strPattern.c_str());
	}

	ComboBox_SetText(hComboBox, m_pwsdps->m_pattern.c_str());

	if (m_selectionType == SelectionType::Deselect)
	{
		std::wstring deselectTitle = m_resourceLoader->LoadString(IDS_WILDCARDDESELECTION);
		SetWindowText(m_hDlg, deselectTitle.c_str());
	}

	SetFocus(hComboBox);

	m_pwsdps->RestoreDialogPosition(m_hDlg, true);

	return 0;
}

std::vector<ResizableDialogControl> WildcardSelectDialog::GetResizableControls()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_SELECTGROUP_COMBOBOX), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(m_hDlg, IDOK), MovingType::Horizontal, SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDCANCEL), MovingType::Horizontal, SizingType::None);
	return controls;
}

INT_PTR WildcardSelectDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam))
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

void WildcardSelectDialog::OnShellBrowserDestroyed()
{
	EndDialog(m_hDlg, 0);
}

void WildcardSelectDialog::OnOk()
{
	TCHAR szPattern[512];
	GetDlgItemText(m_hDlg, IDC_SELECTGROUP_COMBOBOX, szPattern, std::size(szPattern));

	if (lstrlen(szPattern) != 0)
	{
		m_shellBrowser->SelectItemsMatchingPattern(szPattern, m_selectionType);

		bool bStorePattern = true;

		/* If the current text isn't the same as the
		most recent text (if any), add it to the history
		list. */
		auto itr = m_pwsdps->m_PatternList.begin();

		if (itr != m_pwsdps->m_PatternList.end())
		{
			if (lstrcmp(itr->c_str(), szPattern) == 0)
			{
				bStorePattern = false;
			}
		}

		if (bStorePattern)
		{
			m_pwsdps->m_PatternList.push_front(szPattern);
		}
	}

	EndDialog(m_hDlg, 1);
}

void WildcardSelectDialog::OnCancel()
{
	EndDialog(m_hDlg, 0);
}

INT_PTR WildcardSelectDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
	return 0;
}

void WildcardSelectDialog::SaveState()
{
	m_pwsdps->SaveDialogPosition(m_hDlg);

	m_pwsdps->m_pattern = GetDlgItemString(m_hDlg, IDC_SELECTGROUP_COMBOBOX);

	m_pwsdps->m_bStateSaved = TRUE;
}

WildcardSelectDialogPersistentSettings::WildcardSelectDialogPersistentSettings() :
	DialogSettings(SETTINGS_KEY)
{
}

WildcardSelectDialogPersistentSettings &WildcardSelectDialogPersistentSettings::GetInstance()
{
	static WildcardSelectDialogPersistentSettings wsdps;
	return wsdps;
}

void WildcardSelectDialogPersistentSettings::SaveExtraRegistrySettings(HKEY hKey)
{
	RegistrySettings::SaveStringList(hKey, SETTING_PATTERN_LIST, m_PatternList);
	RegistrySettings::SaveString(hKey, SETTING_CURRENT_TEXT, m_pattern);
}

void WildcardSelectDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	RegistrySettings::ReadStringList(hKey, SETTING_PATTERN_LIST, m_PatternList);
	RegistrySettings::ReadString(hKey, SETTING_CURRENT_TEXT, m_pattern);
}

void WildcardSelectDialogPersistentSettings::SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom,
	IXMLDOMElement *pParentNode)
{
	XMLSettings::AddStringListToNode(pXMLDom, pParentNode, SETTING_PATTERN_LIST, m_PatternList);
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_CURRENT_TEXT, m_pattern.c_str());
}

void WildcardSelectDialogPersistentSettings::LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue)
{
	if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, bstrName, lstrlen(SETTING_PATTERN_LIST),
			SETTING_PATTERN_LIST, lstrlen(SETTING_PATTERN_LIST))
		== CSTR_EQUAL)
	{
		m_PatternList.emplace_back(bstrValue);
	}
	else if (lstrcmpi(bstrName, SETTING_CURRENT_TEXT) == 0)
	{
		m_pattern = bstrValue;
	}
}
