/******************************************************************
 *
 * Project: Explorer++
 * File: WildcardSelectDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Allows items to be selected/deselected based
 * on a wildcard filter.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "WildcardSelectDialog.h"
#include "MainResource.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/Helper.h"
#include "../Helper/Registry.h"
#include "../Helper/XMLSettings.h"


const TCHAR CWildcardSelectDialogPersistentSettings::SETTINGS_KEY[] = _T("WildcardSelect");

CWildcardSelectDialog::CWildcardSelectDialog(HINSTANCE hInstance,
	int iResource,HWND hParent,BOOL bSelect) :
CBaseDialog(hInstance,iResource,hParent)
{
	m_bSelect = bSelect;

	m_pwsdps = &CWildcardSelectDialogPersistentSettings::GetInstance();
}

CWildcardSelectDialog::~CWildcardSelectDialog()
{

}

BOOL CWildcardSelectDialog::OnInitDialog()
{
	HWND hComboBox = GetDlgItem(m_hDlg,IDC_SELECTGROUP_COMBOBOX);

	for each(auto strPattern in m_pwsdps->m_PatternList)
	{
		ComboBox_InsertString(hComboBox,-1,strPattern.c_str());
	}

	ComboBox_SetText(hComboBox,m_pwsdps->m_szPattern);

	if(m_pwsdps->m_bStateSaved)
	{
		SetWindowPos(m_hDlg,NULL,m_pwsdps->m_ptDialog.x,
			m_pwsdps->m_ptDialog.y,0,0,SWP_NOSIZE|SWP_NOZORDER);
	}
	else
	{
		CenterWindow(GetParent(m_hDlg),m_hDlg);
	}

	if(!m_bSelect)
	{
		TCHAR szTemp[64];
		LoadString(GetInstance(),IDS_WILDCARDDESELECTION,
			szTemp,SIZEOF_ARRAY(szTemp));
		SetWindowText(m_hDlg,szTemp);
	}

	SetFocus(hComboBox);

	return 0;
}

BOOL CWildcardSelectDialog::OnCommand(WPARAM wParam,LPARAM lParam)
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

void CWildcardSelectDialog::OnOk()
{
	TCHAR szPattern[512];

	GetDlgItemText(m_hDlg,IDC_SELECTGROUP_COMBOBOX,
		szPattern,SIZEOF_ARRAY(szPattern));

	if(lstrcmp(szPattern,EMPTY_STRING) != 0)
	{
		/* TODO: */
		/*int nItems = ListView_GetItemCount(m_hActiveListView);

		for(int i = 0;i < nItems;i++)
		{
			TCHAR	FullFileName[MAX_PATH];

			m_pActiveShellBrowser->QueryName(i,FullFileName);

			if(CheckWildcardMatch(szPattern,FullFileName,FALSE) == 1)
			{
				ListView_SelectItem(m_hActiveListView,i,m_bSelect);
			}
		}*/

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

void CWildcardSelectDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

BOOL CWildcardSelectDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

BOOL CWildcardSelectDialog::OnDestroy()
{
	SaveState();
	return 0;
}

void CWildcardSelectDialog::SaveState()
{
	RECT rc;
	GetWindowRect(m_hDlg,&rc);
	m_pwsdps->m_ptDialog.x = rc.left;
	m_pwsdps->m_ptDialog.y = rc.top;

	HWND hComboBox = GetDlgItem(m_hDlg,IDC_SELECTGROUP_COMBOBOX);
	ComboBox_GetText(hComboBox,m_pwsdps->m_szPattern,SIZEOF_ARRAY(m_pwsdps->m_szPattern));

	m_pwsdps->m_bStateSaved = TRUE;
}

CWildcardSelectDialogPersistentSettings::CWildcardSelectDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{
	StringCchCopy(m_szPattern,SIZEOF_ARRAY(m_szPattern),EMPTY_STRING);
}

CWildcardSelectDialogPersistentSettings::~CWildcardSelectDialogPersistentSettings()
{
	
}

CWildcardSelectDialogPersistentSettings& CWildcardSelectDialogPersistentSettings::GetInstance()
{
	static CWildcardSelectDialogPersistentSettings wsdps;
	return wsdps;
}

void CWildcardSelectDialogPersistentSettings::SaveExtraRegistrySettings(HKEY hKey)
{
	SaveStringListToRegistry(hKey,_T("Pattern"),m_PatternList);
	SaveStringToRegistry(hKey,_T("CurrentText"),m_szPattern);
}

void CWildcardSelectDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	ReadStringListFromRegistry(hKey,_T("Pattern"),m_PatternList);
	ReadStringFromRegistry(hKey,_T("CurrentText"),m_szPattern,
		SIZEOF_ARRAY(m_szPattern));
}

void CWildcardSelectDialogPersistentSettings::SaveExtraXMLSettings(
	MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pParentNode)
{
	TCHAR szNode[64];
	int i = 0;

	for each(auto strPattern in m_PatternList)
	{
		StringCchPrintf(szNode,SIZEOF_ARRAY(szNode),_T("Pattern%d"),i++);
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,szNode,strPattern.c_str());
	}

	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("CurrentText"),m_szPattern);
}

void CWildcardSelectDialogPersistentSettings::LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue)
{
	if(lstrcmpi(bstrName,_T("CurrentText")) == 0)
	{
		StringCchCopy(m_szPattern,SIZEOF_ARRAY(m_szPattern),bstrValue);
	}
	else if(CheckWildcardMatch(_T("Pattern*"),bstrName,TRUE))
	{
		m_PatternList.push_back(bstrValue);
	}
}