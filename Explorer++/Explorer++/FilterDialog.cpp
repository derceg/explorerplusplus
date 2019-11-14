// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FilterDialog.h"
#include "Explorer++_internal.h"
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ShellBrowser/iShellView.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/XMLSettings.h"
#include <list>

const TCHAR CFilterDialogPersistentSettings::SETTINGS_KEY[] = _T("Filter");

const TCHAR CFilterDialogPersistentSettings::SETTING_FILTER_LIST[] = _T("Filter");

CFilterDialog::CFilterDialog(HINSTANCE hInstance,
	int iResource,HWND hParent,IExplorerplusplus *pexpp) :
CBaseDialog(hInstance,iResource,hParent,true)
{
	m_pexpp = pexpp;

	m_pfdps = &CFilterDialogPersistentSettings::GetInstance();
}

CFilterDialog::~CFilterDialog()
{

}

INT_PTR CFilterDialog::OnInitDialog()
{
	HWND hComboBox = GetDlgItem(m_hDlg,IDC_FILTER_COMBOBOX);

	SetFocus(hComboBox);

	for(const auto &strFilter : m_pfdps->m_FilterList)
	{
		SendMessage(hComboBox,CB_ADDSTRING,static_cast<WPARAM>(-1),
			reinterpret_cast<LPARAM>(strFilter.c_str()));
	}

	TCHAR szFilter[512];
	m_pexpp->GetActiveShellBrowser()->GetFilter(szFilter,SIZEOF_ARRAY(szFilter));

	ComboBox_SelectString(hComboBox,-1,szFilter);

	SendMessage(hComboBox,CB_SETEDITSEL,0,MAKELPARAM(0,-1));

	if (m_pexpp->GetActiveShellBrowser()->GetFilterCaseSensitive())
		CheckDlgButton(m_hDlg,IDC_FILTERS_CASESENSITIVE,BST_CHECKED);

	m_pfdps->RestoreDialogPosition(m_hDlg,true);

	return 0;
}

wil::unique_hicon CFilterDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return IconResourceLoader::LoadIconFromPNGAndScale(Icon::Filter, iconWidth, iconHeight);
}

void CFilterDialog::GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc,
	std::list<CResizableDialog::Control_t> &ControlList)
{
	dsc = CBaseDialog::DIALOG_SIZE_CONSTRAINT_X;

	CResizableDialog::Control_t Control;

	Control.iID = IDC_FILTER_COMBOBOX;
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

INT_PTR CFilterDialog::OnCommand(WPARAM wParam,LPARAM lParam)
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

INT_PTR CFilterDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

void CFilterDialog::OnOk()
{
	HWND hComboBox = GetDlgItem(m_hDlg,IDC_FILTER_COMBOBOX);

	int iBufSize = GetWindowTextLength(hComboBox);

	auto filter = std::make_unique<TCHAR[]>(iBufSize + 1);

	SendMessage(hComboBox,WM_GETTEXT,iBufSize + 1,
		reinterpret_cast<LPARAM>(filter.get()));

	bool bFound = false;

	/* If the entry already exists in the list,
	simply move the existing entry to the start.
	Otherwise, insert it at the start. */
	for(auto itr = m_pfdps->m_FilterList.begin();itr != m_pfdps->m_FilterList.end();itr++)
	{
		if(lstrcmp(filter.get(),itr->c_str()) == 0)
		{
			std::iter_swap(itr,m_pfdps->m_FilterList.begin());

			bFound = true;
			break;
		}
	}

	if(!bFound)
	{
		m_pfdps->m_FilterList.push_front(filter.get());
	}

	m_pexpp->GetActiveShellBrowser()->SetFilterCaseSensitive(IsDlgButtonChecked(
		m_hDlg,IDC_FILTERS_CASESENSITIVE) == BST_CHECKED);

	m_pexpp->GetActiveShellBrowser()->SetFilter(filter.get());

	if(!m_pexpp->GetActiveShellBrowser()->GetFilterStatus())
		m_pexpp->GetActiveShellBrowser()->SetFilterStatus(TRUE);

	EndDialog(m_hDlg,1);
}

void CFilterDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

void CFilterDialog::SaveState()
{
	m_pfdps->SaveDialogPosition(m_hDlg);

	m_pfdps->m_bStateSaved = TRUE;
}

CFilterDialogPersistentSettings::CFilterDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{

}

CFilterDialogPersistentSettings::~CFilterDialogPersistentSettings()
{
	
}

CFilterDialogPersistentSettings& CFilterDialogPersistentSettings::GetInstance()
{
	static CFilterDialogPersistentSettings sfadps;
	return sfadps;
}

void CFilterDialogPersistentSettings::SaveExtraRegistrySettings(HKEY hKey)
{
	NRegistrySettings::SaveStringListToRegistry(hKey, SETTING_FILTER_LIST, m_FilterList);
}

void CFilterDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	NRegistrySettings::ReadStringListFromRegistry(hKey, SETTING_FILTER_LIST, m_FilterList);
}

void CFilterDialogPersistentSettings::SaveExtraXMLSettings(
	IXMLDOMDocument *pXMLDom,IXMLDOMElement *pParentNode)
{
	NXMLSettings::AddStringListToNode(pXMLDom, pParentNode, SETTING_FILTER_LIST, m_FilterList);
}

void CFilterDialogPersistentSettings::LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue)
{
	if(CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, bstrName, lstrlen(SETTING_FILTER_LIST),
		SETTING_FILTER_LIST, lstrlen(SETTING_FILTER_LIST)) == CSTR_EQUAL)
	{
		m_FilterList.push_back(bstrValue);
	}
}