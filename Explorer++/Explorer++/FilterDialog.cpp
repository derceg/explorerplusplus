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
#include "Explorer++_internal.h"
#include "FilterDialog.h"
#include "MainResource.h"
#include "../Helper/Helper.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/XMLSettings.h"


const TCHAR CFilterDialogPersistentSettings::SETTINGS_KEY[] = _T("Filter");

CFilterDialog::CFilterDialog(HINSTANCE hInstance,
	int iResource,HWND hParent,IExplorerplusplus *pexpp) :
CBaseDialog(hInstance,iResource,hParent)
{
	m_pexpp = pexpp;

	m_pfdps = &CFilterDialogPersistentSettings::GetInstance();
}

CFilterDialog::~CFilterDialog()
{

}

BOOL CFilterDialog::OnInitDialog()
{
	HIMAGELIST himl = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);
	HBITMAP hBitmap = LoadBitmap(GetInstance(),MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(himl,hBitmap,NULL);

	m_hDialogIcon = ImageList_GetIcon(himl,SHELLIMAGES_FILTER,ILD_NORMAL);
	SetClassLongPtr(m_hDlg,GCLP_HICONSM,reinterpret_cast<LONG_PTR>(m_hDialogIcon));

	HWND hComboBox = GetDlgItem(m_hDlg,IDC_FILTER_COMBOBOX);

	SetFocus(hComboBox);

	for each(auto strFilter in m_pfdps->m_FilterList)
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

	RECT rcMain;
	GetWindowRect(m_hDlg,&rcMain);
	m_iMinWidth = GetRectWidth(&rcMain);
	m_iMinHeight = GetRectHeight(&rcMain);

	m_hGripper = CreateWindow(_T("SCROLLBAR"),EMPTY_STRING,WS_CHILD|WS_VISIBLE|
		WS_CLIPSIBLINGS|SBS_BOTTOMALIGN|SBS_SIZEGRIP,0,0,0,0,m_hDlg,NULL,
		GetInstance(),NULL);

	RECT rc;
	GetClientRect(m_hDlg,&rcMain);
	GetWindowRect(m_hGripper,&rc);
	SetWindowPos(m_hGripper,NULL,GetRectWidth(&rcMain) - GetRectWidth(&rc),
		GetRectHeight(&rcMain) - GetRectHeight(&rc),0,0,SWP_NOSIZE|SWP_NOZORDER);

	InitializeControlStates();

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

void CFilterDialog::InitializeControlStates()
{
	std::list<CResizableDialog::Control_t> ControlList;
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

	m_prd = new CResizableDialog(m_hDlg,ControlList);
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

BOOL CFilterDialog::OnGetMinMaxInfo(LPMINMAXINFO pmmi)
{
	pmmi->ptMinTrackSize.x = m_iMinWidth;
	pmmi->ptMinTrackSize.y = m_iMinHeight;

	return 0;
}

BOOL CFilterDialog::OnSize(int iType,int iWidth,int iHeight)
{
	RECT rc;
	GetWindowRect(m_hGripper,&rc);
	SetWindowPos(m_hGripper,NULL,iWidth - GetRectWidth(&rc),iHeight - GetRectHeight(&rc),0,
		0,SWP_NOSIZE|SWP_NOZORDER);

	m_prd->UpdateControls(iWidth,iHeight);

	return 0;
}

BOOL CFilterDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

BOOL CFilterDialog::OnDestroy()
{
	DestroyIcon(m_hDialogIcon);

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

	bool bFound = false;

	/* If the entry already exists in the list,
	simply move the existing entry to the start.
	Otherwise, insert it at the start. */
	for(auto itr = m_pfdps->m_FilterList.begin();itr != m_pfdps->m_FilterList.end();itr++)
	{
		if(lstrcmp(pszFilter,itr->c_str()) == 0)
		{
			std::iter_swap(itr,m_pfdps->m_FilterList.begin());

			bFound = true;
			break;
		}
	}

	if(!bFound)
	{
		m_pfdps->m_FilterList.push_front(pszFilter);
	}

	m_pexpp->GetActiveShellBrowser()->SetFilterCaseSensitive(IsDlgButtonChecked(
		m_hDlg,IDC_FILTERS_CASESENSITIVE) == BST_CHECKED);

	m_pexpp->GetActiveShellBrowser()->SetFilter(pszFilter);

	if(!m_pexpp->GetActiveShellBrowser()->GetFilterStatus())
		m_pexpp->GetActiveShellBrowser()->SetFilterStatus(TRUE);

	delete[] pszFilter;

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