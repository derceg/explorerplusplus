// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "WildcardSelectDialog.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "ShellBrowser/ShellBrowser.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/XMLSettings.h"

const TCHAR CWildcardSelectDialogPersistentSettings::SETTINGS_KEY[] = _T("WildcardSelect");

const TCHAR CWildcardSelectDialogPersistentSettings::SETTING_PATTERN_LIST[] = _T("Pattern");
const TCHAR CWildcardSelectDialogPersistentSettings::SETTING_CURRENT_TEXT[] = _T("CurrentText");

CWildcardSelectDialog::CWildcardSelectDialog(HINSTANCE hInstance, HWND hParent,
	BOOL bSelect, IExplorerplusplus *pexpp) :
	CBaseDialog(hInstance, IDD_WILDCARDSELECT, hParent, true)
{
	m_bSelect = bSelect;
	m_pexpp = pexpp;

	m_pwsdps = &CWildcardSelectDialogPersistentSettings::GetInstance();
}

INT_PTR CWildcardSelectDialog::OnInitDialog()
{
	m_icon.reset(LoadIcon(GetModuleHandle(0),MAKEINTRESOURCE(IDI_MAIN)));
	SetClassLongPtr(m_hDlg,GCLP_HICONSM,reinterpret_cast<LONG_PTR>(m_icon.get()));

	HWND hComboBox = GetDlgItem(m_hDlg,IDC_SELECTGROUP_COMBOBOX);

	for(const auto &strPattern : m_pwsdps->m_PatternList)
	{
		ComboBox_InsertString(hComboBox,-1,strPattern.c_str());
	}

	ComboBox_SetText(hComboBox,m_pwsdps->m_szPattern);

	if(!m_bSelect)
	{
		TCHAR szTemp[64];
		LoadString(GetInstance(),IDS_WILDCARDDESELECTION,
			szTemp,SIZEOF_ARRAY(szTemp));
		SetWindowText(m_hDlg,szTemp);
	}

	SetFocus(hComboBox);

	m_pwsdps->RestoreDialogPosition(m_hDlg,true);

	return 0;
}

void CWildcardSelectDialog::GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc,
	std::list<CResizableDialog::Control_t> &ControlList)
{
	dsc = CBaseDialog::DIALOG_SIZE_CONSTRAINT_X;

	CResizableDialog::Control_t Control;

	Control.iID = IDC_SELECTGROUP_COMBOBOX;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDOK;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDCANCEL;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDC_GRIPPER;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);
}

INT_PTR CWildcardSelectDialog::OnCommand(WPARAM wParam,LPARAM lParam)
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

void CWildcardSelectDialog::OnOk()
{
	TCHAR szPattern[512];

	GetDlgItemText(m_hDlg,IDC_SELECTGROUP_COMBOBOX,
		szPattern,SIZEOF_ARRAY(szPattern));

	if(lstrcmp(szPattern,EMPTY_STRING) != 0)
	{
		SelectItems(szPattern);

		bool bStorePattern = true;

		/* If the current text isn't the same as the
		most recent text (if any), add it to the history
		list. */
		auto itr = m_pwsdps->m_PatternList.begin();

		if(itr != m_pwsdps->m_PatternList.end())
		{
			if(lstrcmp(itr->c_str(),szPattern) == 0)
			{
				bStorePattern = false;
			}
		}

		if(bStorePattern)
		{
			m_pwsdps->m_PatternList.push_front(szPattern);
		}
	}

	EndDialog(m_hDlg,1);
}

void CWildcardSelectDialog::SelectItems(TCHAR *szPattern)
{
	HWND hListView = m_pexpp->GetActiveListView();

	int nItems = ListView_GetItemCount(hListView);

	for(int i = 0;i < nItems;i++)
	{
		TCHAR szFilename[MAX_PATH];
		m_pexpp->GetActiveShellBrowser()->GetItemDisplayName(i,SIZEOF_ARRAY(szFilename),szFilename);

		if(CheckWildcardMatch(szPattern,szFilename,FALSE) == 1)
		{
			NListView::ListView_SelectItem(hListView,i,m_bSelect);
		}
	}
}

void CWildcardSelectDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

INT_PTR CWildcardSelectDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

void CWildcardSelectDialog::SaveState()
{
	m_pwsdps->SaveDialogPosition(m_hDlg);

	HWND hComboBox = GetDlgItem(m_hDlg,IDC_SELECTGROUP_COMBOBOX);
	ComboBox_GetText(hComboBox,m_pwsdps->m_szPattern,SIZEOF_ARRAY(m_pwsdps->m_szPattern));

	m_pwsdps->m_bStateSaved = TRUE;
}

CWildcardSelectDialogPersistentSettings::CWildcardSelectDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{
	StringCchCopy(m_szPattern,SIZEOF_ARRAY(m_szPattern),EMPTY_STRING);
}

CWildcardSelectDialogPersistentSettings& CWildcardSelectDialogPersistentSettings::GetInstance()
{
	static CWildcardSelectDialogPersistentSettings wsdps;
	return wsdps;
}

void CWildcardSelectDialogPersistentSettings::SaveExtraRegistrySettings(HKEY hKey)
{
	NRegistrySettings::SaveStringListToRegistry(hKey, SETTING_PATTERN_LIST, m_PatternList);
	NRegistrySettings::SaveStringToRegistry(hKey, SETTING_CURRENT_TEXT, m_szPattern);
}

void CWildcardSelectDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	NRegistrySettings::ReadStringListFromRegistry(hKey, SETTING_PATTERN_LIST, m_PatternList);
	NRegistrySettings::ReadStringFromRegistry(hKey, SETTING_CURRENT_TEXT, m_szPattern,
		SIZEOF_ARRAY(m_szPattern));
}

void CWildcardSelectDialogPersistentSettings::SaveExtraXMLSettings(
	IXMLDOMDocument *pXMLDom,IXMLDOMElement *pParentNode)
{
	NXMLSettings::AddStringListToNode(pXMLDom, pParentNode, SETTING_PATTERN_LIST, m_PatternList);
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_CURRENT_TEXT, m_szPattern);
}

void CWildcardSelectDialogPersistentSettings::LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue)
{
	if(CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, bstrName, lstrlen(SETTING_PATTERN_LIST),
		SETTING_PATTERN_LIST, lstrlen(SETTING_PATTERN_LIST)) == CSTR_EQUAL)
	{
		m_PatternList.push_back(bstrValue);
	}
	else if(lstrcmpi(bstrName, SETTING_CURRENT_TEXT) == 0)
	{
		StringCchCopy(m_szPattern,SIZEOF_ARRAY(m_szPattern),bstrValue);
	}
}