// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SelectColumnsDialog.h"
#include "Explorer++_internal.h"
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ShellBrowser/iShellView.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include <algorithm>
#include <functional>

const TCHAR CSelectColumnsDialogPersistentSettings::SETTINGS_KEY[] = _T("SelectColumns");

CSelectColumnsDialog::CSelectColumnsDialog(HINSTANCE hInstance,
	int iResource,HWND hParent,IExplorerplusplus *pexpp,
	TabContainer *tabContainer, TabInterface *ti) :
	CBaseDialog(hInstance,iResource,hParent,true),
	m_pexpp(pexpp),
	m_tabContainer(tabContainer),
	m_ti(ti),
	m_bColumnsSwapped(FALSE)
{
	m_pscdps = &CSelectColumnsDialogPersistentSettings::GetInstance();
}

CSelectColumnsDialog::~CSelectColumnsDialog()
{

}

INT_PTR CSelectColumnsDialog::OnInitDialog()
{
	UINT dpi = m_dpiCompat.GetDpiForWindow(m_hDlg);
	m_icon = IconResourceLoader::LoadIconFromPNGForDpi(Icon::SelectColumns, DIALOG_ICON_SIZE_96DPI, dpi);
	SetClassLongPtr(m_hDlg,GCLP_HICONSM,reinterpret_cast<LONG_PTR>(m_icon.get()));

	HWND hListView = GetDlgItem(m_hDlg,IDC_COLUMNS_LISTVIEW);
	SetWindowTheme(hListView,L"Explorer",NULL);

	ListView_SetExtendedListViewStyleEx(hListView,
		LVS_EX_CHECKBOXES,LVS_EX_CHECKBOXES);

	LVCOLUMN lvColumn;
	lvColumn.mask = 0;
	ListView_InsertColumn(hListView,0,&lvColumn);

	auto currentColumns = m_pexpp->GetActiveShellBrowser()->ExportCurrentColumns();

	std::sort(currentColumns.begin(), currentColumns.end(),
		std::bind(&CSelectColumnsDialog::CompareColumns, this, std::placeholders::_1, std::placeholders::_2));

	int iItem = 0;

	for(const auto &column : currentColumns)
	{
		TCHAR szText[64];
		LoadString(GetInstance(),CShellBrowser::LookupColumnNameStringIndex(column.id),
			szText,SIZEOF_ARRAY(szText));

		LVITEM lvItem;
		lvItem.mask		= LVIF_TEXT|LVIF_PARAM;
		lvItem.iItem	= iItem;
		lvItem.iSubItem	= 0;
		lvItem.pszText	= szText;
		lvItem.lParam	= column.id;
		ListView_InsertItem(hListView,&lvItem);

		ListView_SetCheckState(hListView,iItem,column.bChecked);

		iItem++;
	}

	ListView_SetColumnWidth(hListView,0,LVSCW_AUTOSIZE);

	NListView::ListView_SelectItem(hListView,0,TRUE);
	SetFocus(hListView);

	m_pscdps->RestoreDialogPosition(m_hDlg,true);

	return 0;
}

bool CSelectColumnsDialog::CompareColumns(const Column_t &column1, const Column_t &column2)
{
	if (column1.bChecked && column2.bChecked)
	{
		// If both column are checked, preserve the input ordering (this is
		// the order that the columns will actually appear in the listview).
		// This matches the behavior of Windows Explorer.
		return false;
	}
	else if (column1.bChecked && !column2.bChecked)
	{
		return true;
	}
	else if (!column1.bChecked && column2.bChecked)
	{
		return false;
	}

	TCHAR column1Text[64];
	LoadString(GetInstance(), CShellBrowser::LookupColumnNameStringIndex(column1.id),
		column1Text, SIZEOF_ARRAY(column1Text));

	TCHAR column2Text[64];
	LoadString(GetInstance(), CShellBrowser::LookupColumnNameStringIndex(column2.id),
		column2Text, SIZEOF_ARRAY(column2Text));

	int ret = StrCmpLogicalW(column1Text, column2Text);

	if (ret == -1)
	{
		return true;
	}
	else
	{
		return false;
	}
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

INT_PTR CSelectColumnsDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

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

INT_PTR CSelectColumnsDialog::OnNotify(NMHDR *pnmhdr)
{
	switch(pnmhdr->code)
	{
	case LVN_ITEMCHANGED:
		OnLvnItemChanged(reinterpret_cast<NMLISTVIEW *>(pnmhdr));
		break;
	}

	return 0;
}

INT_PTR CSelectColumnsDialog::OnClose()
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
	HWND hListView = GetDlgItem(m_hDlg,IDC_COLUMNS_LISTVIEW);
	std::vector<Column_t> updatedColumns;

	auto currentColumns = m_pexpp->GetActiveShellBrowser()->ExportCurrentColumns();

	for(int i = 0;i < ListView_GetItemCount(hListView);i++)
	{
		LVITEM lvItem;
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= i;
		lvItem.iSubItem	= 0;
		ListView_GetItem(hListView,&lvItem);

		UINT id = static_cast<int>(lvItem.lParam);
		auto itr = std::find_if(currentColumns.begin(),currentColumns.end(),
			[id](const Column_t &Column){return Column.id == id;});

		Column_t Column;
		Column.id		= id;
		Column.iWidth	= itr->iWidth;
		Column.bChecked	= ListView_GetCheckState(hListView,i);
		updatedColumns.push_back(Column);
	}

	m_pexpp->GetActiveShellBrowser()->ImportColumns(updatedColumns);

	if(m_bColumnsSwapped)
	{
		m_ti->RefreshTab(m_tabContainer->GetSelectedTab());
	}

	EndDialog(m_hDlg,1);
}

void CSelectColumnsDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

void CSelectColumnsDialog::OnLvnItemChanged(NMLISTVIEW *pnmlv)
{
	if(pnmlv->uNewState & LVIS_SELECTED)
	{
		HWND hListView = GetDlgItem(m_hDlg,IDC_COLUMNS_LISTVIEW);

		LVITEM lvItem;
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= pnmlv->iItem;
		lvItem.iSubItem	= 0;
		ListView_GetItem(hListView,&lvItem);

		int iDescriptionStringIndex = CShellBrowser::LookupColumnDescriptionStringIndex(static_cast<int>(lvItem.lParam));

		TCHAR szColumnDescription[128];
		LoadString(GetInstance(),iDescriptionStringIndex,szColumnDescription,
			SIZEOF_ARRAY(szColumnDescription));
		SetDlgItemText(m_hDlg,IDC_COLUMNS_DESCRIPTION,szColumnDescription);
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
			NListView::ListView_SwapItems(hListView,iSelected,iSelected - 1,TRUE);
		}
		else
		{
			NListView::ListView_SwapItems(hListView,iSelected,iSelected + 1,TRUE);
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