/******************************************************************
 *
 * Project: Explorer++
 * File: SelectColumnsDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the 'Select Columns' dialog.
 *
 * Methodology:
 * Each column set will be copied to a
 * temporary on initialization. If the
 * 'OK' button is used to exit the dialog,
 * the temporary column copies will be
 * set as the new defaults; if 'Cancel' is
 * used to ecit the dialog, nothing
 * permanent will happen.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"


list<Column_t>	g_RealFolderColumnList;
list<Column_t>	g_MyComputerColumnList;
list<Column_t>	g_ControlPanelColumnList;
list<Column_t>	g_RecycleBinColumnList;
list<Column_t>	g_PrintersColumnList;
list<Column_t>	g_NetworkConnectionsColumnList;
list<Column_t>	g_MyNetworkPlacesColumnList;

DWORD g_iControlPanel;
DWORD g_iGeneral;
DWORD g_iMyComputer;
DWORD g_iNetwork;
DWORD g_iNetworkPlaces;
DWORD g_iPrinters;
DWORD g_iRecycleBin;
DWORD g_iPreviousTypeSel;

int g_nDefaultColumnsChecked = 0;

INT_PTR CALLBACK SetDefaultColumnsProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static CContainer *pContainer = NULL;

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			pContainer = (CContainer *)lParam;
		}
		break;
	}

	return pContainer->SetDefaultColumnsProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK CContainer::SetDefaultColumnsProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
		case WM_INITDIALOG:
			OnSetDefaultColumnsInit(hDlg);
			break;

		case WM_COMMAND:
			switch(HIWORD(wParam))
			{
				case CBN_SELCHANGE:
					OnSetDefaultColumnsCBChange(hDlg);
					break;
			}

			switch(LOWORD(wParam))
			{
				case IDC_DEFAULTCOLUMNS_MOVEUP:
					MoveColumnItem2(hDlg,TRUE);
					break;

				case IDC_DEFAULTCOLUMNS_MOVEDOWN:
					MoveColumnItem2(hDlg,FALSE);
					break;

				case IDC_DEFAULTCOLUMNS_ENABLE:
					EnableColumnItem(hDlg,TRUE);
					break;

				case IDC_DEFAULTCOLUMNS_DISABLE:
					EnableColumnItem(hDlg,FALSE);
					break;

				case IDOK:
					OnSetDefaultColumnsOk(hDlg);
					break;

				case IDCANCEL:
					SetDefaultColumnsSaveState(hDlg);
					EndDialog(hDlg,0);
					break;
			}
			break;

		case WM_NOTIFY:
			{
				NMHDR *nmhdr;

				nmhdr = (NMHDR *)lParam;

				switch(nmhdr->code)
				{
					case LVN_ITEMCHANGING:
						OnSetDefaultColumnsLvnItemChanging(hDlg,lParam);
						break;
				}
			}
			break;

		case WM_CLOSE:
			SetDefaultColumnsSaveState(hDlg);
			EndDialog(hDlg,0);
			break;
	}

	return 0;
}

void CContainer::OnSetDefaultColumnsInit(HWND hDlg)
{
	HWND	hComboBox;
	TCHAR	szFolderName[MAX_PATH];

	if(m_bSetDefaultColumnsDlgStateSaved)
	{
		SetWindowPos(hDlg,NULL,m_ptSetDefaultColumns.x,
			m_ptSetDefaultColumns.y,0,0,SWP_NOSIZE|SWP_NOZORDER);
	}
	else
	{
		CenterWindow(m_hContainer,hDlg);
	}

	hComboBox = GetDlgItem(hDlg,IDC_DEFAULTCOLUMNS_COMBOBOX);

	/* Need to be careful here. If the combobox decides to insert
	items in the first position, then the indexes will be wrong.
	Similarly, if the CBS_SORT window style is applied, items
	will be inserted in sorted order, causing the indexes to
	overlap.
	Should use CB_INSERTSTRING. */
	GetCsidlFolderName(CSIDL_CONTROLS,szFolderName,SHGDN_INFOLDER);
	g_iControlPanel		= (DWORD)SendMessage(hComboBox,CB_ADDSTRING,0,(LPARAM)szFolderName);

	LoadString(g_hLanguageModule,IDS_DEFAULTCOLUMNS_GENERAL,szFolderName,SIZEOF_ARRAY(szFolderName));
	g_iGeneral			= (DWORD)SendMessage(hComboBox,CB_ADDSTRING,0,(LPARAM)szFolderName);

	GetCsidlFolderName(CSIDL_DRIVES,szFolderName,SHGDN_INFOLDER);
	g_iMyComputer		= (DWORD)SendMessage(hComboBox,CB_ADDSTRING,0,(LPARAM)szFolderName);

	GetCsidlFolderName(CSIDL_CONNECTIONS,szFolderName,SHGDN_INFOLDER);
	g_iNetwork			= (DWORD)SendMessage(hComboBox,CB_ADDSTRING,0,(LPARAM)szFolderName);

	GetCsidlFolderName(CSIDL_NETWORK,szFolderName,SHGDN_INFOLDER);
	g_iNetworkPlaces	= (DWORD)SendMessage(hComboBox,CB_ADDSTRING,0,(LPARAM)szFolderName);

	GetCsidlFolderName(CSIDL_PRINTERS,szFolderName,SHGDN_INFOLDER);
	g_iPrinters			= (DWORD)SendMessage(hComboBox,CB_ADDSTRING,0,(LPARAM)szFolderName);

	GetCsidlFolderName(CSIDL_BITBUCKET,szFolderName,SHGDN_INFOLDER);
	g_iRecycleBin		= (DWORD)SendMessage(hComboBox,CB_ADDSTRING,0,(LPARAM)szFolderName);

	SendMessage(hComboBox,CB_SETCURSEL,1,0);
	g_iPreviousTypeSel = 1;

	HWND						hListView;
	list<Column_t>				*pColumnList = NULL;
	list<Column_t>::iterator	itr;
	LVITEM						lvItem;
	LVCOLUMN					lvColumn;
	TCHAR						szText[64];
	int							iItem = 0;

	OnSetDefaultColumnsInitialzeTempColumns();

	hListView = GetDlgItem(hDlg,IDC_DEFAULTCOLUMNS_LISTVIEW);

	ListView_SetExtendedListViewStyleEx(hListView,
	LVS_EX_CHECKBOXES,LVS_EX_CHECKBOXES);

	lvColumn.mask	= LVCF_WIDTH;
	lvColumn.cx		= 180;
	ListView_InsertColumn(hListView,0,&lvColumn);

	pColumnList = &m_RealFolderColumnList;

	/* MUST set this beforehand. If it is 1 while items are
	been inserted, their checkbox may not appear and the
	dialog may appear corrupted. */
	//g_nDefaultColumnsChecked = 0;

	for(itr = pColumnList->begin();itr != pColumnList->end();itr++)
	{
		LoadString(g_hLanguageModule,LookupColumnNameStringIndex(itr->id),szText,SIZEOF_ARRAY(szText));

		lvItem.mask		= LVIF_TEXT | LVIF_PARAM;
		lvItem.iItem	= iItem;
		lvItem.iSubItem	= 0;
		lvItem.pszText	= szText;
		lvItem.lParam	= itr->id;

		ListView_InsertItem(hListView,&lvItem);

		ListView_SetCheckState(hListView,iItem,itr->bChecked);

		iItem++;
	}

	ListView_SelectItem(hListView,0,TRUE);
	SetFocus(hListView);
}

void CContainer::OnSetDefaultColumnsInitialzeTempColumns(void)
{
	g_RealFolderColumnList			= m_RealFolderColumnList;
	g_MyComputerColumnList			= m_MyComputerColumnList;
	g_ControlPanelColumnList		= m_ControlPanelColumnList;
	g_RecycleBinColumnList			= m_RecycleBinColumnList;
	g_PrintersColumnList			= m_PrintersColumnList;
	g_NetworkConnectionsColumnList	= m_NetworkConnectionsColumnList;
	g_MyNetworkPlacesColumnList		= m_MyNetworkPlacesColumnList;
}

void CContainer::OnSetDefaultColumnsExportTempColumns(void)
{
	m_RealFolderColumnList			= g_RealFolderColumnList;
	m_MyComputerColumnList			= g_MyComputerColumnList;
	m_ControlPanelColumnList		= g_ControlPanelColumnList;
	m_RecycleBinColumnList			= g_RecycleBinColumnList;
	m_PrintersColumnList			= g_PrintersColumnList;
	m_NetworkConnectionsColumnList	= g_NetworkConnectionsColumnList;
	m_MyNetworkPlacesColumnList		= g_MyNetworkPlacesColumnList;
}

void CContainer::OnSetDefaultColumnsCBChange(HWND hDlg)
{
	HWND						hComboBox;
	HWND						hListView;
	list<Column_t>::iterator	itr;
	LVITEM						lvItem;
	TCHAR						szText[64];
	int							iItem = 0;
	list<Column_t>				*pColumnList = NULL;
	DWORD						iSelected;

	hComboBox = GetDlgItem(hDlg,IDC_DEFAULTCOLUMNS_COMBOBOX);

	/* Save the state of the current items. */
	GetCurrentDefaultColumnState(hDlg);

	iSelected = (int)SendMessage(hComboBox,CB_GETCURSEL,0,0);

	if(iSelected == g_iControlPanel)
		pColumnList = &g_ControlPanelColumnList;
	else if(iSelected == g_iGeneral)
		pColumnList = &g_RealFolderColumnList;
	else if(iSelected == g_iMyComputer)
		pColumnList = &g_MyComputerColumnList;
	else if(iSelected == g_iNetwork)
		pColumnList = &g_NetworkConnectionsColumnList;
	else if(iSelected == g_iNetworkPlaces)
		pColumnList = &g_MyNetworkPlacesColumnList;
	else if(iSelected == g_iPrinters)
		pColumnList = &g_PrintersColumnList;
	else if(iSelected == g_iRecycleBin)
		pColumnList = &g_RecycleBinColumnList;

	g_iPreviousTypeSel = iSelected;

	hListView = GetDlgItem(hDlg,IDC_DEFAULTCOLUMNS_LISTVIEW);

	ListView_DeleteAllItems(hListView);

	for(itr = pColumnList->begin();itr != pColumnList->end();itr++)
	{
		LoadString(g_hLanguageModule,LookupColumnNameStringIndex(itr->id),szText,SIZEOF_ARRAY(szText));

		lvItem.mask		= LVIF_TEXT | LVIF_PARAM;
		lvItem.iItem	= iItem;
		lvItem.iSubItem	= 0;
		lvItem.pszText	= szText;
		lvItem.lParam	= itr->id;

		ListView_InsertItem(hListView,&lvItem);

		ListView_SetCheckState(hListView,iItem,itr->bChecked);

		iItem++;
	}

	ListView_SelectItem(hListView,0,TRUE);
}

void CContainer::GetCurrentDefaultColumnState(HWND hDlg)
{
	HWND			hComboBox;
	HWND			hListView;
	LVITEM			lvItem;
	list<Column_t>	*pColumnList = NULL;
	Column_t		Column;
	int				i = 0;

	hComboBox = GetDlgItem(hDlg,IDC_DEFAULTCOLUMNS_COMBOBOX);
	hListView = GetDlgItem(hDlg,IDC_DEFAULTCOLUMNS_LISTVIEW);

	if(g_iPreviousTypeSel == g_iControlPanel)
		pColumnList = &g_ControlPanelColumnList;
	else if(g_iPreviousTypeSel == g_iGeneral)
		pColumnList = &g_RealFolderColumnList;
	else if(g_iPreviousTypeSel == g_iMyComputer)
		pColumnList = &g_MyComputerColumnList;
	else if(g_iPreviousTypeSel == g_iNetwork)
		pColumnList = &g_NetworkConnectionsColumnList;
	else if(g_iPreviousTypeSel == g_iNetworkPlaces)
		pColumnList = &g_MyNetworkPlacesColumnList;
	else if(g_iPreviousTypeSel == g_iPrinters)
		pColumnList = &g_PrintersColumnList;
	else if(g_iPreviousTypeSel == g_iRecycleBin)
		pColumnList = &g_RecycleBinColumnList;

	pColumnList->clear();

	for(i = 0;i < ListView_GetItemCount(hListView);i++)
	{
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= i;
		lvItem.iSubItem	= 0;
		ListView_GetItem(hListView,&lvItem);

		/* TODO: May need to pull the current column width (instead
		pf using the default width). */
		Column.id		= (int)lvItem.lParam;
		Column.iWidth	= DEFAULT_COLUMN_WIDTH;
		Column.bChecked	= ListView_GetCheckState(hListView,i);
		pColumnList->push_back(Column);
	}
}

BOOL CContainer::OnSetDefaultColumnsLvnItemChanging(HWND hDlg,LPARAM lParam)
{
	NMLISTVIEW *nmlv;
	HWND hListView;

	nmlv = (NMLISTVIEW *)lParam;

	hListView = GetDlgItem(hDlg,IDC_COLUMNS_LISTVIEW);

	if((nmlv->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(2))
	{
		/* The item was checked. */
		EnableWindow(GetDlgItem(hDlg,IDC_DEFAULTCOLUMNS_ENABLE),FALSE);
		EnableWindow(GetDlgItem(hDlg,IDC_DEFAULTCOLUMNS_DISABLE),TRUE);

		g_nDefaultColumnsChecked++;
	}
	else if((nmlv->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(1))
	{
		/* The item was unchecked. */

		if(g_nDefaultColumnsChecked != 1)
		{
			EnableWindow(GetDlgItem(hDlg,IDC_DEFAULTCOLUMNS_DISABLE),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_DEFAULTCOLUMNS_ENABLE),TRUE);

			g_nDefaultColumnsChecked--;
		}
		else
		{
			/* Can't just return. Need to set the DWL_MSGRESULT
			value to the return value first. */
			SetWindowLongPtr(hDlg,DWLP_MSGRESULT,TRUE);
			return TRUE;
		}
	}
	else if(nmlv->uNewState & LVIS_SELECTED)
	{
		LVITEM lvItem;
		TCHAR szColumnDescription[128];
		int iDescriptionStringIndex;

		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= nmlv->iItem;
		lvItem.iSubItem	= 0;

		ListView_GetItem(hListView,&lvItem);

		iDescriptionStringIndex = LookupColumnDescriptionStringIndex((int)lvItem.lParam);

		LoadString(g_hLanguageModule,iDescriptionStringIndex,szColumnDescription,
		SIZEOF_ARRAY(szColumnDescription));
		SetDlgItemText(hDlg,IDC_COLUMNS_DESCRIPTION,szColumnDescription);

		if(ListView_GetCheckState(hListView,nmlv->iItem))
		{
			EnableWindow(GetDlgItem(hDlg,IDC_DEFAULTCOLUMNS_ENABLE),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_DEFAULTCOLUMNS_DISABLE),TRUE);
		}
		else
		{
			EnableWindow(GetDlgItem(hDlg,IDC_DEFAULTCOLUMNS_ENABLE),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_DEFAULTCOLUMNS_DISABLE),TRUE);
		}
	}

	return FALSE;
}

void CContainer::OnSetDefaultColumnsOk(HWND hDlg)
{
	/* Save the state of the current items. */
	GetCurrentDefaultColumnState(hDlg);

	OnSetDefaultColumnsExportTempColumns();

	SetDefaultColumnsSaveState(hDlg);
	EndDialog(hDlg,1);
}

void CContainer::SetDefaultColumnsSaveState(HWND hDlg)
{
	RECT rcTemp;

	GetWindowRect(hDlg,&rcTemp);
	m_ptSetDefaultColumns.x = rcTemp.left;
	m_ptSetDefaultColumns.y = rcTemp.top;

	m_bSetDefaultColumnsDlgStateSaved = TRUE;
}