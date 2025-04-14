// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FilterDialog.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ResourceLoader.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/XMLSettings.h"
#include <list>

const TCHAR FilterDialogPersistentSettings::SETTINGS_KEY[] = _T("Filter");

const TCHAR FilterDialogPersistentSettings::SETTING_FILTER_LIST[] = _T("Filter");

FilterDialog::FilterDialog(const ResourceLoader *resourceLoader, HINSTANCE resourceInstance,
	HWND hParent, ThemeManager *themeManager, CoreInterface *coreInterface) :
	ThemedDialog(resourceLoader, resourceInstance, IDD_FILTER, hParent,
		DialogSizingType::Horizontal, themeManager),
	m_coreInterface(coreInterface)
{
	m_persistentSettings = &FilterDialogPersistentSettings::GetInstance();
}

INT_PTR FilterDialog::OnInitDialog()
{
	HWND hComboBox = GetDlgItem(m_hDlg, IDC_FILTER_COMBOBOX);

	SetFocus(hComboBox);

	for (const auto &strFilter : m_persistentSettings->m_FilterList)
	{
		SendMessage(hComboBox, CB_ADDSTRING, static_cast<WPARAM>(-1),
			reinterpret_cast<LPARAM>(strFilter.c_str()));
	}

	std::wstring filter = m_coreInterface->GetActiveShellBrowserImpl()->GetFilterText();

	ComboBox_SelectString(hComboBox, -1, filter.c_str());

	SendMessage(hComboBox, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));

	if (m_coreInterface->GetActiveShellBrowserImpl()->GetFilterCaseSensitive())
	{
		CheckDlgButton(m_hDlg, IDC_FILTERS_CASESENSITIVE, BST_CHECKED);
	}

	m_persistentSettings->RestoreDialogPosition(m_hDlg, true);

	return 0;
}

wil::unique_hicon FilterDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return m_resourceLoader->LoadIconFromPNGAndScale(Icon::Filter, iconWidth, iconHeight);
}

std::vector<ResizableDialogControl> FilterDialog::GetResizableControls()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_FILTER_COMBOBOX), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_FILTERS_CASESENSITIVE), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(m_hDlg, IDOK), MovingType::Horizontal, SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDCANCEL), MovingType::Horizontal, SizingType::None);
	return controls;
}

INT_PTR FilterDialog::OnCommand(WPARAM wParam, LPARAM lParam)
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

INT_PTR FilterDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
	return 0;
}

void FilterDialog::OnOk()
{
	HWND hComboBox = GetDlgItem(m_hDlg, IDC_FILTER_COMBOBOX);

	std::wstring filter = GetWindowString(hComboBox);

	bool bFound = false;

	/* If the entry already exists in the list,
	simply move the existing entry to the start.
	Otherwise, insert it at the start. */
	for (auto itr = m_persistentSettings->m_FilterList.begin();
		itr != m_persistentSettings->m_FilterList.end(); itr++)
	{
		if (filter == *itr)
		{
			std::iter_swap(itr, m_persistentSettings->m_FilterList.begin());

			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		m_persistentSettings->m_FilterList.push_front(filter);
	}

	m_coreInterface->GetActiveShellBrowserImpl()->SetFilterCaseSensitive(
		IsDlgButtonChecked(m_hDlg, IDC_FILTERS_CASESENSITIVE) == BST_CHECKED);

	m_coreInterface->GetActiveShellBrowserImpl()->SetFilterText(filter);

	if (!m_coreInterface->GetActiveShellBrowserImpl()->IsFilterApplied())
	{
		m_coreInterface->GetActiveShellBrowserImpl()->SetFilterApplied(TRUE);
	}

	EndDialog(m_hDlg, 1);
}

void FilterDialog::OnCancel()
{
	EndDialog(m_hDlg, 0);
}

void FilterDialog::SaveState()
{
	m_persistentSettings->SaveDialogPosition(m_hDlg);

	m_persistentSettings->m_bStateSaved = TRUE;
}

FilterDialogPersistentSettings::FilterDialogPersistentSettings() : DialogSettings(SETTINGS_KEY)
{
}

FilterDialogPersistentSettings &FilterDialogPersistentSettings::GetInstance()
{
	static FilterDialogPersistentSettings sfadps;
	return sfadps;
}

void FilterDialogPersistentSettings::SaveExtraRegistrySettings(HKEY hKey)
{
	RegistrySettings::SaveStringList(hKey, SETTING_FILTER_LIST, m_FilterList);
}

void FilterDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	RegistrySettings::ReadStringList(hKey, SETTING_FILTER_LIST, m_FilterList);
}

void FilterDialogPersistentSettings::SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom,
	IXMLDOMElement *pParentNode)
{
	XMLSettings::AddStringListToNode(pXMLDom, pParentNode, SETTING_FILTER_LIST, m_FilterList);
}

void FilterDialogPersistentSettings::LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue)
{
	if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, bstrName, lstrlen(SETTING_FILTER_LIST),
			SETTING_FILTER_LIST, lstrlen(SETTING_FILTER_LIST))
		== CSTR_EQUAL)
	{
		m_FilterList.emplace_back(bstrValue);
	}
}
