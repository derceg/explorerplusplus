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
#include "XMLSettings.h"
#include "MainResource.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/Helper.h"
#include "../Helper/Registry.h"


const TCHAR CWildcardSelectDialogPersistentSettings::REGISTRY_SETTINGS_KEY[] = _T("WildcardSelect");

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

	m_pwsdps->m_PatternList.push_front(szPattern);

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

CWildcardSelectDialogPersistentSettings::CWildcardSelectDialogPersistentSettings()
{
	m_bStateSaved = FALSE;

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

void CWildcardSelectDialogPersistentSettings::SaveSettings(HKEY hParentKey)
{
	if(!m_bStateSaved)
	{
		return;
	}

	HKEY hKey;
	DWORD dwDisposition;

	LONG lRes = RegCreateKeyEx(hParentKey,REGISTRY_SETTINGS_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&dwDisposition);

	if(lRes == ERROR_SUCCESS)
	{
		RegSetValueEx(hKey,_T("Position"),0,REG_BINARY,
			reinterpret_cast<LPBYTE>(&m_ptDialog),
			sizeof(m_ptDialog));

		TCHAR szItemKey[128];
		int i = 0;

		for each(auto strPattern in m_PatternList)
		{
			StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),_T("Pattern%d"),i++);
			SaveStringToRegistry(hKey,szItemKey,strPattern.c_str());
		}

		SaveStringToRegistry(hKey,_T("CurrentText"),m_szPattern);

		RegCloseKey(hKey);
	}
}

void CWildcardSelectDialogPersistentSettings::LoadSettings(HKEY hParentKey)
{
	HKEY hKey;
	TCHAR szItemKey[128];
	LONG lRes;

	lRes = RegOpenKeyEx(hParentKey,REGISTRY_SETTINGS_KEY,0,
		KEY_READ,&hKey);

	if(lRes == ERROR_SUCCESS)
	{
		DWORD dwSize = sizeof(POINT);
		RegQueryValueEx(hKey,_T("Position"),
			NULL,NULL,(LPBYTE)&m_ptDialog,&dwSize);

		TCHAR szTemp[512];
		int i = 0;

		lRes = ERROR_SUCCESS;

		while(lRes == ERROR_SUCCESS)
		{
			StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),
				_T("Pattern%d"),i++);

			lRes = ReadStringFromRegistry(hKey,szItemKey,
				szTemp,SIZEOF_ARRAY(szTemp));

			if(lRes == ERROR_SUCCESS)
			{
				m_PatternList.push_back(szTemp);
			}
		}

		ReadStringFromRegistry(hKey,_T("CurrentText"),
			m_szPattern,SIZEOF_ARRAY(m_szPattern));

		m_bStateSaved = TRUE;

		RegCloseKey(hKey);
	}
}

void CWildcardSelectDialogPersistentSettings::SaveSettings(MSXML2::IXMLDOMDocument *pXMLDom,
	MSXML2::IXMLDOMElement *pe)
{
	if(!m_bStateSaved)
	{
		return;
	}

	MSXML2::IXMLDOMElement *pParentNode = NULL;
	BSTR bstr_wsntt = SysAllocString(L"\n\t\t");

	AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);

	/* TODO: Move node name into constant. */
	CreateElementNode(pXMLDom,&pParentNode,pe,_T("DialogState"),_T("WildcardSelect"));

	TCHAR szNode[64];
	int i = 0;

	for each(auto strPattern in m_PatternList)
	{
		StringCchPrintf(szNode,SIZEOF_ARRAY(szNode),_T("Pattern%d"),i++);
		AddAttributeToNode(pXMLDom,pParentNode,szNode,strPattern.c_str());
	}

	AddAttributeToNode(pXMLDom,pParentNode,_T("CurrentText"),m_szPattern);
}

void CWildcardSelectDialogPersistentSettings::LoadSettings(MSXML2::IXMLDOMNamedNodeMap *pam,
	long lChildNodes)
{
	MSXML2::IXMLDOMNode *pNode = NULL;
	BSTR bstrName;
	BSTR bstrValue;

	for(int i = 1;i < lChildNodes;i++)
	{
		pam->get_item(i,&pNode);

		pNode->get_nodeName(&bstrName);
		pNode->get_text(&bstrValue);

		if(lstrcmpi(bstrName,_T("CurrentText")) == 0)
		{
			StringCchCopy(m_szPattern,SIZEOF_ARRAY(m_szPattern),bstrValue);
		}
		else if(CheckWildcardMatch(_T("Pattern*"),bstrName,TRUE))
		{
			m_PatternList.push_back(bstrValue);
		}
	}
}