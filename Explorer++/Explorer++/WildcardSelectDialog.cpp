// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "WildcardSelectDialog.h"
#include "BrowserPane.h"
#include "BrowserWindow.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "TabContainerImpl.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/XMLSettings.h"

const TCHAR WildcardSelectDialogPersistentSettings::SETTINGS_KEY[] = _T("WildcardSelect");

const TCHAR WildcardSelectDialogPersistentSettings::SETTING_PATTERN_LIST[] = _T("Pattern");
const TCHAR WildcardSelectDialogPersistentSettings::SETTING_CURRENT_TEXT[] = _T("CurrentText");

WildcardSelectDialog::WildcardSelectDialog(const ResourceLoader *resourceLoader,
	HINSTANCE resourceInstance, HWND hParent, ThemeManager *themeManager, BOOL bSelect,
	BrowserWindow *browserWindow) :
	ThemedDialog(resourceLoader, resourceInstance, IDD_WILDCARDSELECT, hParent,
		DialogSizingType::Horizontal, themeManager),
	m_bSelect(bSelect),
	m_browserWindow(browserWindow)
{
	m_pwsdps = &WildcardSelectDialogPersistentSettings::GetInstance();
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

	if (!m_bSelect)
	{
		std::wstring deselectTitle =
			ResourceHelper::LoadString(GetResourceInstance(), IDS_WILDCARDDESELECTION);
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

void WildcardSelectDialog::OnOk()
{
	TCHAR szPattern[512];
	GetDlgItemText(m_hDlg, IDC_SELECTGROUP_COMBOBOX, szPattern, std::size(szPattern));

	if (lstrlen(szPattern) != 0)
	{
		SelectItems(szPattern);

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

void WildcardSelectDialog::SelectItems(TCHAR *szPattern)
{
	const auto &tab = m_browserWindow->GetActivePane()->GetTabContainerImpl()->GetSelectedTab();
	HWND hListView = tab.GetShellBrowserImpl()->GetListView();

	int nItems = ListView_GetItemCount(hListView);

	for (int i = 0; i < nItems; i++)
	{
		std::wstring filename = tab.GetShellBrowserImpl()->GetItemName(i);

		if (CheckWildcardMatch(szPattern, filename.c_str(), FALSE) == 1)
		{
			ListViewHelper::SelectItem(hListView, i, m_bSelect);
		}
	}
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
