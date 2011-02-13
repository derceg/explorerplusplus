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
#include "Explorer++_internal.h"
#include "WildcardSelectDialog.h"
#include "MainResource.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/Helper.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/XMLSettings.h"


const TCHAR CWildcardSelectDialogPersistentSettings::SETTINGS_KEY[] = _T("WildcardSelect");

CWildcardSelectDialog::CWildcardSelectDialog(HINSTANCE hInstance,
	int iResource,HWND hParent,BOOL bSelect,IExplorerplusplus *pexpp) :
CBaseDialog(hInstance,iResource,hParent,true)
{
	m_bSelect = bSelect;
	m_pexpp = pexpp;

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

		m_pexpp->GetActiveShellBrowser()->QueryName(i,szFilename);

		if(CheckWildcardMatch(szPattern,szFilename,FALSE) == 1)
		{
			ListView_SelectItem(hListView,i,m_bSelect);
		}
	}
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
	NRegistrySettings::SaveStringListToRegistry(hKey,_T("Pattern"),m_PatternList);
	NRegistrySettings::SaveStringToRegistry(hKey,_T("CurrentText"),m_szPattern);
}

void CWildcardSelectDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	NRegistrySettings::ReadStringListFromRegistry(hKey,_T("Pattern"),m_PatternList);
	NRegistrySettings::ReadStringFromRegistry(hKey,_T("CurrentText"),m_szPattern,
		SIZEOF_ARRAY(m_szPattern));
}

void CWildcardSelectDialogPersistentSettings::SaveExtraXMLSettings(
	MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pParentNode)
{
	NXMLSettings::AddStringListToNode(pXMLDom,pParentNode,_T("Pattern"),m_PatternList);
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