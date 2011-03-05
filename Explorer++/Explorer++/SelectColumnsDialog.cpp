/******************************************************************
 *
 * Project: Explorer++
 * File: SelectColumnsDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the 'Select Columns' dialog.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++_internal.h"
#include "SelectColumnsDialog.h"
#include "MainResource.h"
#include "../ShellBrowser/iShellView.h"
#include "../Helper/Helper.h"


const TCHAR CSelectColumnsDialogPersistentSettings::SETTINGS_KEY[] = _T("SelectColumns");

CSelectColumnsDialog::CSelectColumnsDialog(HINSTANCE hInstance,
	int iResource,HWND hParent) :
CBaseDialog(hInstance,iResource,hParent,true)
{
	m_bColumnsSwapped = FALSE;

	m_pscdps = &CSelectColumnsDialogPersistentSettings::GetInstance();
}

CSelectColumnsDialog::~CSelectColumnsDialog()
{

}

BOOL CSelectColumnsDialog::OnInitDialog()
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_COLUMNS_LISTVIEW);
	SetWindowTheme(hListView,L"Explorer",NULL);

	ListView_SetExtendedListViewStyleEx(hListView,
		LVS_EX_CHECKBOXES,LVS_EX_CHECKBOXES);

	LVCOLUMN lvColumn;
	lvColumn.mask	= LVCF_WIDTH;
	lvColumn.cx		= 180;
	ListView_InsertColumn(hListView,0,&lvColumn);

	/* TODO: */
	//std::list<Column_t> ActiveColumnList;
	//m_pActiveShellBrowser->ExportCurrentColumns(&pActiveColumnList);

	//int iItem = 0;

	//for each(auto Column in ActiveColumnList)
	//{
	//	/* TODO: */
	//	TCHAR szText[64];
	//	/*LoadString(g_hLanguageModule,LookupColumnNameStringIndex(itr->id),
	//		szText,SIZEOF_ARRAY(szText));*/

	//	LVITEM lvItem;
	//	lvItem.mask		= LVIF_TEXT | LVIF_PARAM;
	//	lvItem.iItem	= iItem;
	//	lvItem.iSubItem	= 0;
	//	lvItem.pszText	= szText;
	//	lvItem.lParam	= Column.id;
	//	ListView_InsertItem(hListView,&lvItem);

	//	ListView_SetCheckState(hListView,iItem,Column.bChecked);

	//	iItem++;
	//}

	ListView_SelectItem(hListView,0,TRUE);
	SetFocus(hListView);

	m_pscdps->RestoreDialogPosition(m_hDlg,true);

	return 0;
}

void CSelectColumnsDialog::GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc,
	std::list<CResizableDialog::Control_t> &ControlList)
{
	dsc = CBaseDialog::DIALOG_SIZE_CONSTRAINT_NONE;

	CResizableDialog::Control_t Control;

	Control.iID = IDC_COLUMNS_LISTVIEW;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDC_COLUMNS_MOVEUP;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_COLUMNS_MOVEDOWN;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_STATIC_DESCRIPTION;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(Control);

	Control.iID = IDC_COLUMNS_DESCRIPTION;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(Control);

	Control.iID = IDC_COLUMNS_DESCRIPTION;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_STATIC_ETCHEDHORZ;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(Control);

	Control.iID = IDC_STATIC_ETCHEDHORZ;
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

BOOL CSelectColumnsDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	switch(LOWORD(wParam))
	{
	case IDC_COLUMNS_MOVEUP:
		OnMoveColumn(true);
		break;

	case IDC_COLUMNS_MOVEDOWN:
		OnMoveColumn(false);
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

BOOL CSelectColumnsDialog::OnNotify(NMHDR *pnmhdr)
{
	switch(pnmhdr->code)
	{
	case LVN_ITEMCHANGING:
		OnLvnItemChanging(reinterpret_cast<NMLISTVIEW *>(pnmhdr));
		break;
	}

	return 0;
}

BOOL CSelectColumnsDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

void CSelectColumnsDialog::SaveState()
{
	m_pscdps->SaveDialogPosition(m_hDlg);
	m_pscdps->m_bStateSaved = TRUE;
}

void CSelectColumnsDialog::OnOk()
{
	std::list<Column_t> ColumnTempList;

	HWND hListView = GetDlgItem(m_hDlg,IDC_COLUMNS_LISTVIEW);

	for(int i = 0;i < ListView_GetItemCount(hListView);i++)
	{
		LVITEM lvItem;
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= i;
		lvItem.iSubItem	= 0;
		ListView_GetItem(hListView,&lvItem);

		Column_t Column;
		Column.id		= static_cast<int>(lvItem.lParam);
		Column.bChecked	= ListView_GetCheckState(hListView,i);
		Column.iWidth	= DEFAULT_COLUMN_WIDTH;
		ColumnTempList.push_back(Column);
	}

	/* TODO: */
	/*m_pActiveShellBrowser->ImportColumns(&ColumnTempList,m_bColumnsSwapped);

	if(m_bColumnsSwapped)
	{
		RefreshTab(m_iObjectIndex);
	}*/

	EndDialog(m_hDlg,1);
}

void CSelectColumnsDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

void CSelectColumnsDialog::OnLvnItemChanging(NMLISTVIEW *pnmlv)
{
	if(pnmlv->uNewState & LVIS_SELECTED)
	{
		HWND hListView = GetDlgItem(m_hDlg,IDC_COLUMNS_LISTVIEW);

		LVITEM lvItem;
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= pnmlv->iItem;
		lvItem.iSubItem	= 0;
		ListView_GetItem(hListView,&lvItem);

		/* TODO */
		/*int iDescriptionStringIndex = LookupColumnDescriptionStringIndex((int)lvItem.lParam);

		TCHAR szColumnDescription[128];
		LoadString(GetInstance(),iDescriptionStringIndex,szColumnDescription,
			SIZEOF_ARRAY(szColumnDescription));
		SetDlgItemText(m_hDlg,IDC_COLUMNS_DESCRIPTION,szColumnDescription);*/
	}
}

void CSelectColumnsDialog::OnMoveColumn(bool bUp)
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_COLUMNS_LISTVIEW);

	int iSelected = ListView_GetNextItem(hListView,-1,LVNI_SELECTED);

	if(iSelected != -1)
	{
		if(bUp)
		{
			ListView_SwapItems(hListView,iSelected,iSelected - 1);
		}
		else
		{
			ListView_SwapItems(hListView,iSelected,iSelected + 1);
		}

		m_bColumnsSwapped = TRUE;

		SetFocus(hListView);
	}
}

CSelectColumnsDialogPersistentSettings::CSelectColumnsDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{

}

CSelectColumnsDialogPersistentSettings::~CSelectColumnsDialogPersistentSettings()
{
	
}

CSelectColumnsDialogPersistentSettings& CSelectColumnsDialogPersistentSettings::GetInstance()
{
	static CSelectColumnsDialogPersistentSettings mfdps;
	return mfdps;
}