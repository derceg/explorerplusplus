/******************************************************************
 *
 * Project: Explorer++
 * File: FilterDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the 'Filter' dialog.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "FilterDialog.h"
#include "MainResource.h"
#include "../Helper/Helper.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/XMLSettings.h"


/* TODO: Resizable. */

const TCHAR CFilterDialogPersistentSettings::SETTINGS_KEY[] = _T("Filter");

CFilterDialog::CFilterDialog(HINSTANCE hInstance,
	int iResource,HWND hParent) :
CBaseDialog(hInstance,iResource,hParent)
{
	m_pfdps = &CFilterDialogPersistentSettings::GetInstance();
}

CFilterDialog::~CFilterDialog()
{

}

BOOL CFilterDialog::OnInitDialog()
{
	HWND hComboBox = GetDlgItem(m_hDlg,IDC_FILTER_COMBOBOX);

	SetFocus(hComboBox);

	for each(auto strFilter in m_pfdps->m_FilterList)
	{
		SendMessage(hComboBox,CB_ADDSTRING,static_cast<WPARAM>(-1),
			reinterpret_cast<LPARAM>(strFilter.c_str()));
	}

	/* TODO: */
	/*TCHAR szFilter[256];
	m_pActiveShellBrowser->GetFilter(szFilter,SIZEOF_ARRAY(szFilter));

	ComboBox_SelectString(hComboBox,-1,szFilter);

	SendMessage(hComboBox,CB_SETEDITSEL,0,MAKELPARAM(0,-1));

	if (m_pActiveShellBrowser->GetFilterCaseSensitive())
		CheckDlgButton(m_hDlg,IDC_FILTERS_CASESENSITIVE,BST_CHECKED);*/

	if(m_pfdps->m_bStateSaved)
	{
		SetWindowPos(m_hDlg,NULL,m_pfdps->m_ptDialog.x,
			m_pfdps->m_ptDialog.y,0,0,SWP_NOSIZE|SWP_NOZORDER);
	}
	else
	{
		CenterWindow(GetParent(m_hDlg),m_hDlg);
	}

	return 0;
}

BOOL CFilterDialog::OnCommand(WPARAM wParam,LPARAM lParam)
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

BOOL CFilterDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

BOOL CFilterDialog::OnDestroy()
{
	SaveState();
	return 0;
}

void CFilterDialog::OnOk()
{
	HWND hComboBox = GetDlgItem(m_hDlg,IDC_FILTER_COMBOBOX);

	int iBufSize = GetWindowTextLength(hComboBox);

	TCHAR *pszFilter = new TCHAR[iBufSize + 1];
	SendMessage(hComboBox,WM_GETTEXT,iBufSize + 1,
		reinterpret_cast<LPARAM>(pszFilter));
	m_pfdps->m_FilterList.push_front(pszFilter);
	delete[] pszFilter;

	/* TODO: */
	/*m_pActiveShellBrowser->SetFilterCaseSensitive(IsDlgButtonChecked(
		m_hDlg,IDC_FILTERS_CASESENSITIVE) == BST_CHECKED);

	m_pActiveShellBrowser->SetFilter(pszFilter);

	if(!m_pActiveShellBrowser->GetFilterStatus())
		m_pActiveShellBrowser->SetFilterStatus(TRUE);*/

	EndDialog(m_hDlg,1);
}

void CFilterDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

void CFilterDialog::SaveState()
{
	RECT rc;
	GetWindowRect(m_hDlg,&rc);
	m_pfdps->m_ptDialog.x = rc.left;
	m_pfdps->m_ptDialog.y = rc.top;

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
	NRegistrySettings::SaveStringListToRegistry(hKey,_T("Filter"),m_FilterList);
}

void CFilterDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	NRegistrySettings::ReadStringListFromRegistry(hKey,_T("Filter"),m_FilterList);
}

void CFilterDialogPersistentSettings::SaveExtraXMLSettings(
	MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pParentNode)
{
	NXMLSettings::AddStringListToNode(pXMLDom,pParentNode,_T("Filter"),m_FilterList);
}

void CFilterDialogPersistentSettings::LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue)
{
	if(CheckWildcardMatch(_T("Filter*"),bstrName,TRUE))
	{
		m_FilterList.push_back(bstrValue);
	}
}