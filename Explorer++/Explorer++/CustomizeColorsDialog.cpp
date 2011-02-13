/******************************************************************
 *
 * Project: Explorer++
 * File: CustomizeColorsDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the customize colors dialog.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++_internal.h"
#include "CustomizeColorsDialog.h"
#include "ColorRuleDialog.h"
#include "MainResource.h"
#include "../Helper/Helper.h"


const TCHAR CCustomizeColorsDialogPersistentSettings::SETTINGS_KEY[] = _T("CustomizeColors");

CCustomizeColorsDialog::CCustomizeColorsDialog(HINSTANCE hInstance,
	int iResource,HWND hParent,std::vector<ColorRule_t> *pColorRuleList) :
CBaseDialog(hInstance,iResource,hParent,true)
{
	m_pColorRuleList = pColorRuleList;

	m_pccdps = &CCustomizeColorsDialogPersistentSettings::GetInstance();
}

CCustomizeColorsDialog::~CCustomizeColorsDialog()
{

}

BOOL CCustomizeColorsDialog::OnInitDialog()
{
	HIMAGELIST himl = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);
	HBITMAP hBitmap = LoadBitmap(GetInstance(),MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(himl,hBitmap,NULL);

	m_hDialogIcon = ImageList_GetIcon(himl,SHELLIMAGES_CUSTOMIZECOLORS,ILD_NORMAL);
	SetClassLongPtr(m_hDlg,GCLP_HICONSM,reinterpret_cast<LONG_PTR>(m_hDialogIcon));

	HWND hListView = GetDlgItem(m_hDlg,IDC_LISTVIEW_COLORRULES);

	SetWindowTheme(hListView,L"Explorer",NULL);

	ListView_SetExtendedListViewStyleEx(hListView,
		LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES,
		LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);

	/* TODO: Move text into string table. */
	LVCOLUMN lvColumn;
	lvColumn.mask		= LVCF_TEXT;
	lvColumn.pszText	= _T("Description");
	ListView_InsertColumn(hListView,0,&lvColumn);

	lvColumn.mask		= LVCF_TEXT;
	lvColumn.pszText	= _T("Filename Pattern");
	ListView_InsertColumn(hListView,1,&lvColumn);

	lvColumn.mask		= LVCF_TEXT;
	lvColumn.pszText	= _T("Attributes");
	ListView_InsertColumn(hListView,2,&lvColumn);

	RECT rc;
	GetClientRect(hListView,&rc);
	SendMessage(hListView,LVM_SETCOLUMNWIDTH,0,GetRectWidth(&rc) / 3);
	SendMessage(hListView,LVM_SETCOLUMNWIDTH,1,GetRectWidth(&rc) / 3);
	SendMessage(hListView,LVM_SETCOLUMNWIDTH,2,GetRectWidth(&rc) / 3);

	int iItem = 0;

	for each(auto ColorRule in *m_pColorRuleList)
	{
		InsertColorRuleIntoListView(hListView,ColorRule,iItem++);
	}

	SetFocus(hListView);

	m_pccdps->RestoreDialogPosition(m_hDlg,true);

	return 0;
}

void CCustomizeColorsDialog::GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc,
	std::list<CResizableDialog::Control_t> &ControlList)
{
	dsc = CBaseDialog::DIALOG_SIZE_CONSTRAINT_NONE;

	CResizableDialog::Control_t Control;

	Control.iID = IDC_LISTVIEW_COLORRULES;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDC_BUTTON_DELETE;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_BUTTON_MOVEDOWN;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_BUTTON_MOVEUP;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_BUTTON_EDIT;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_BUTTON_NEW;
	Control.Type = CResizableDialog::TYPE_MOVE;
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

void CCustomizeColorsDialog::InsertColorRuleIntoListView(HWND hListView,const ColorRule_t &ColorRule,
	int iIndex)
{
	TCHAR szTemp[512];

	StringCchCopy(szTemp,SIZEOF_ARRAY(szTemp),
		ColorRule.strDescription.c_str());

	LVITEM lvItem;
	lvItem.mask		= LVIF_TEXT;
	lvItem.pszText	= szTemp;
	lvItem.iItem	= iIndex;
	lvItem.iSubItem	= 0;
	int iActualIndex = ListView_InsertItem(hListView,&lvItem);

	if(iActualIndex != -1)
	{
		StringCchCopy(szTemp,SIZEOF_ARRAY(szTemp),ColorRule.strFilterPattern.c_str());
		ListView_SetItemText(hListView,iActualIndex,1,szTemp);

		BuildFileAttributeStringInternal(ColorRule.dwFilterAttributes,szTemp,SIZEOF_ARRAY(szTemp));
		ListView_SetItemText(hListView,iActualIndex,2,szTemp);
	}
}

BOOL CCustomizeColorsDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	switch(LOWORD(wParam))
	{
	case IDC_BUTTON_NEW:
		OnNew();
		break;

	case IDC_BUTTON_EDIT:
		OnEdit();
		break;

	case IDC_BUTTON_MOVEUP:
		OnMove(TRUE);
		break;

	case IDC_BUTTON_MOVEDOWN:
		OnMove(FALSE);
		break;

	case IDC_BUTTON_DELETE:
		OnDelete();
		break;

	case IDOK:
		OnOk();
		break;

	case IDCANCEL:
		OnCancel();
		break;
	}

	return 0;
}

BOOL CCustomizeColorsDialog::OnNotify(NMHDR *pnmhdr)
{
	switch(pnmhdr->code)
	{
	case NM_DBLCLK:
		{
			NMITEMACTIVATE *pnmItem = reinterpret_cast<NMITEMACTIVATE *>(pnmhdr);

			if(pnmItem->iItem != -1)
			{
				EditColorRule(pnmItem->iItem);
			}
		}
		break;
	}

	return 0;
}

BOOL CCustomizeColorsDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

BOOL CCustomizeColorsDialog::OnDestroy()
{
	DestroyIcon(m_hDialogIcon);

	return 0;
}

void CCustomizeColorsDialog::SaveState()
{
	m_pccdps->SaveDialogPosition(m_hDlg);

	m_pccdps->m_bStateSaved = TRUE;
}

void CCustomizeColorsDialog::OnNew()
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_LISTVIEW_COLORRULES);

	ColorRule_t ColorRule;

	CColorRuleDialog ColorRuleDialog(GetInstance(),IDD_NEWCOLORRULE,m_hDlg,&ColorRule,FALSE);

	INT_PTR iRet = ColorRuleDialog.ShowModalDialog();

	if(iRet == 1)
	{
		m_pColorRuleList->push_back(ColorRule);

		int nItems = ListView_GetItemCount(hListView);
		InsertColorRuleIntoListView(hListView,ColorRule,nItems);
	}

	SetFocus(m_hDlg);
}

void CCustomizeColorsDialog::OnEdit()
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_LISTVIEW_COLORRULES);
	int iSelected = ListView_GetNextItem(hListView,-1,LVNI_ALL|LVNI_SELECTED);

	if(iSelected != -1)
	{
		EditColorRule(iSelected);
	}

	SetFocus(m_hDlg);
}

void CCustomizeColorsDialog::EditColorRule(int iSelected)
{
	CColorRuleDialog ColorRuleDialog(GetInstance(),IDD_NEWCOLORRULE,m_hDlg,
		&(*m_pColorRuleList)[iSelected],TRUE);

	INT_PTR iRet = ColorRuleDialog.ShowModalDialog();

	if(iRet == 1)
	{
		HWND hListView = GetDlgItem(m_hDlg,IDC_LISTVIEW_COLORRULES);

		TCHAR szTemp[512];

		StringCchCopy(szTemp,SIZEOF_ARRAY(szTemp),
			(*m_pColorRuleList)[iSelected].strDescription.c_str());
		ListView_SetItemText(hListView,iSelected,0,szTemp);

		StringCchCopy(szTemp,SIZEOF_ARRAY(szTemp),
			(*m_pColorRuleList)[iSelected].strFilterPattern.c_str());
		ListView_SetItemText(hListView,iSelected,1,szTemp);

		BuildFileAttributeStringInternal((*m_pColorRuleList)[iSelected].dwFilterAttributes,
			szTemp,SIZEOF_ARRAY(szTemp));
		ListView_SetItemText(hListView,iSelected,2,szTemp);
	}
}

void CCustomizeColorsDialog::OnMove(BOOL bUp)
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_LISTVIEW_COLORRULES);
	int iSelected = ListView_GetNextItem(hListView,-1,LVNI_SELECTED);

	if(iSelected != -1)
	{
		int iSwap;

		if(bUp)
		{
			if(iSelected == 0)
				return;

			iSwap = iSelected - 1;
		}
		else
		{
			if(iSelected == static_cast<int>((m_pColorRuleList->size() - 1)))
				return;

			iSwap = iSelected + 1;
		}

		auto itrSelected = m_pColorRuleList->begin();
		std::advance(itrSelected,iSelected);

		auto itrSwap = m_pColorRuleList->begin();
		std::advance(itrSelected,iSwap);

		std::iter_swap(itrSelected,itrSwap);

		ListView_SwapItemsNolParam(hListView,iSelected,iSwap);
	}
}

void CCustomizeColorsDialog::OnDelete()
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_LISTVIEW_COLORRULES);
	int iSelected = ListView_GetNextItem(hListView,-1,LVNI_SELECTED);

	if(iSelected != -1)
	{
		TCHAR szInfoMsg[128];
		LoadString(GetInstance(),IDS_COLORRULE_DELETE,
			szInfoMsg,SIZEOF_ARRAY(szInfoMsg));

		int iRes = MessageBox(m_hDlg,szInfoMsg,
			NExplorerplusplus::WINDOW_NAME,MB_YESNO|MB_ICONINFORMATION|MB_DEFBUTTON2);

		if(iRes == IDYES)
		{
			int nItems = static_cast<int>(m_pColorRuleList->size());

			std::vector<ColorRule_t>::iterator itr = m_pColorRuleList->begin();
			std::advance(itr,iSelected);
			m_pColorRuleList->erase(itr);

			ListView_DeleteItem(hListView,iSelected);

			if(iSelected == (nItems - 1))
				iSelected--;

			ListView_SelectItem(hListView,iSelected,TRUE);
		}

		SetFocus(hListView);
	}
}

void CCustomizeColorsDialog::OnOk()
{
	EndDialog(m_hDlg,1);
}

void CCustomizeColorsDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

CCustomizeColorsDialogPersistentSettings::CCustomizeColorsDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{

}

CCustomizeColorsDialogPersistentSettings::~CCustomizeColorsDialogPersistentSettings()
{
	
}

CCustomizeColorsDialogPersistentSettings& CCustomizeColorsDialogPersistentSettings::GetInstance()
{
	static CCustomizeColorsDialogPersistentSettings sfadps;
	return sfadps;
}