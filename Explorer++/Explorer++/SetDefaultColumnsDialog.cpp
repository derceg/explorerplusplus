// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SetDefaultColumnsDialog.h"
#include "MainResource.h"
#include "ShellBrowser/ShellBrowser.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/XMLSettings.h"
#include <algorithm>

const TCHAR SetDefaultColumnsDialogPersistentSettings::SETTINGS_KEY[] = _T("SetDefaultColumns");

const TCHAR SetDefaultColumnsDialogPersistentSettings::SETTING_FOLDER_TYPE[] = _T("Folder");

SetDefaultColumnsDialog::SetDefaultColumnsDialog(HINSTANCE hInstance, HWND hParent,
	FolderColumns &folderColumns) :
	BaseDialog(hInstance, IDD_SETDEFAULTCOLUMNS, hParent, true),
	m_folderColumns(folderColumns)
{
	m_psdcdps = &SetDefaultColumnsDialogPersistentSettings::GetInstance();
}

INT_PTR SetDefaultColumnsDialog::OnInitDialog()
{
	m_icon.reset(LoadIcon(GetModuleHandle(nullptr),MAKEINTRESOURCE(IDI_MAIN)));
	SetClassLongPtr(m_hDlg,GCLP_HICONSM,reinterpret_cast<LONG_PTR>(m_icon.get()));

	HWND hComboBox = GetDlgItem(m_hDlg,IDC_DEFAULTCOLUMNS_COMBOBOX);

	TCHAR szFolderName[MAX_PATH];
	int iPos;

	GetCsidlDisplayName(CSIDL_CONTROLS,szFolderName,SIZEOF_ARRAY(szFolderName),SHGDN_INFOLDER);
	iPos = static_cast<int>(SendMessage(hComboBox,CB_INSERTSTRING,static_cast<WPARAM>(-1),reinterpret_cast<LPARAM>(szFolderName)));
	m_FolderMap.insert(std::unordered_map<int,FolderType_t>::value_type(iPos,FOLDER_TYPE_CONTROL_PANEL));

	LoadString(GetInstance(),IDS_DEFAULTCOLUMNS_GENERAL,szFolderName,SIZEOF_ARRAY(szFolderName));
	iPos = static_cast<int>(SendMessage(hComboBox,CB_INSERTSTRING,static_cast<WPARAM>(-1),reinterpret_cast<LPARAM>(szFolderName)));
	m_FolderMap.insert(std::unordered_map<int,FolderType_t>::value_type(iPos,FOLDER_TYPE_GENERAL));

	GetCsidlDisplayName(CSIDL_DRIVES,szFolderName,SIZEOF_ARRAY(szFolderName),SHGDN_INFOLDER);
	iPos = static_cast<int>(SendMessage(hComboBox,CB_INSERTSTRING,static_cast<WPARAM>(-1),reinterpret_cast<LPARAM>(szFolderName)));
	m_FolderMap.insert(std::unordered_map<int,FolderType_t>::value_type(iPos,FOLDER_TYPE_COMPUTER));

	GetCsidlDisplayName(CSIDL_CONNECTIONS,szFolderName,SIZEOF_ARRAY(szFolderName),SHGDN_INFOLDER);
	iPos = static_cast<int>(SendMessage(hComboBox,CB_INSERTSTRING,static_cast<WPARAM>(-1),reinterpret_cast<LPARAM>(szFolderName)));
	m_FolderMap.insert(std::unordered_map<int,FolderType_t>::value_type(iPos,FOLDER_TYPE_NETWORK));

	GetCsidlDisplayName(CSIDL_NETWORK,szFolderName,SIZEOF_ARRAY(szFolderName),SHGDN_INFOLDER);
	iPos = static_cast<int>(SendMessage(hComboBox,CB_INSERTSTRING,static_cast<WPARAM>(-1),reinterpret_cast<LPARAM>(szFolderName)));
	m_FolderMap.insert(std::unordered_map<int,FolderType_t>::value_type(iPos,FOLDER_TYPE_NETWORK_PLACES));

	GetCsidlDisplayName(CSIDL_PRINTERS,szFolderName,SIZEOF_ARRAY(szFolderName),SHGDN_INFOLDER);
	iPos = static_cast<int>(SendMessage(hComboBox,CB_INSERTSTRING,static_cast<WPARAM>(-1),reinterpret_cast<LPARAM>(szFolderName)));
	m_FolderMap.insert(std::unordered_map<int,FolderType_t>::value_type(iPos,FOLDER_TYPE_PRINTERS));

	GetCsidlDisplayName(CSIDL_BITBUCKET,szFolderName,SIZEOF_ARRAY(szFolderName),SHGDN_INFOLDER);
	iPos = static_cast<int>(SendMessage(hComboBox,CB_INSERTSTRING,static_cast<WPARAM>(-1),reinterpret_cast<LPARAM>(szFolderName)));
	m_FolderMap.insert(std::unordered_map<int,FolderType_t>::value_type(iPos,FOLDER_TYPE_RECYCLE_BIN));

	auto FolderType = m_psdcdps->m_FolderType;
	auto itr = std::find_if(m_FolderMap.begin(),m_FolderMap.end(),
		[FolderType](const std::unordered_map<int,FolderType_t>::value_type &vt){return vt.second == FolderType;});
	SendMessage(hComboBox,CB_SETCURSEL,itr->first,0);

	m_PreviousFolderType = m_psdcdps->m_FolderType;

	HWND hListView = GetDlgItem(m_hDlg,IDC_DEFAULTCOLUMNS_LISTVIEW);
	SetWindowTheme(hListView, L"Explorer", nullptr);

	ListView_SetExtendedListViewStyleEx(hListView,
	LVS_EX_CHECKBOXES,LVS_EX_CHECKBOXES);

	LVCOLUMN lvColumn;
	lvColumn.mask = LVCF_WIDTH;
	lvColumn.cx = 180;
	ListView_InsertColumn(hListView,0,&lvColumn);

	SetupFolderColumns(m_psdcdps->m_FolderType);

	SetFocus(hListView);

	m_psdcdps->RestoreDialogPosition(m_hDlg,true);

	return 0;
}

void SetDefaultColumnsDialog::GetResizableControlInformation(BaseDialog::DialogSizeConstraint &dsc,
	std::list<ResizableDialog::Control_t> &ControlList)
{
	dsc = BaseDialog::DIALOG_SIZE_CONSTRAINT_NONE;

	ResizableDialog::Control_t Control;

	Control.iID = IDC_COLUMNS_LISTVIEW;
	Control.Type = ResizableDialog::TYPE_RESIZE;
	Control.Constraint = ResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDC_COLUMNS_MOVEUP;
	Control.Type = ResizableDialog::TYPE_MOVE;
	Control.Constraint = ResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_COLUMNS_MOVEDOWN;
	Control.Type = ResizableDialog::TYPE_MOVE;
	Control.Constraint = ResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_STATIC_DESCRIPTION;
	Control.Type = ResizableDialog::TYPE_MOVE;
	Control.Constraint = ResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(Control);

	Control.iID = IDC_DEFAULTCOLUMNS_DESCRIPTION;
	Control.Type = ResizableDialog::TYPE_MOVE;
	Control.Constraint = ResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(Control);

	Control.iID = IDC_DEFAULTCOLUMNS_DESCRIPTION;
	Control.Type = ResizableDialog::TYPE_RESIZE;
	Control.Constraint = ResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_STATIC_ETCHEDHORZ;
	Control.Type = ResizableDialog::TYPE_MOVE;
	Control.Constraint = ResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(Control);

	Control.iID = IDC_STATIC_ETCHEDHORZ;
	Control.Type = ResizableDialog::TYPE_RESIZE;
	Control.Constraint = ResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDOK;
	Control.Type = ResizableDialog::TYPE_MOVE;
	Control.Constraint = ResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDCANCEL;
	Control.Type = ResizableDialog::TYPE_MOVE;
	Control.Constraint = ResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDC_GRIPPER;
	Control.Type = ResizableDialog::TYPE_MOVE;
	Control.Constraint = ResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);
}

INT_PTR SetDefaultColumnsDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch(HIWORD(wParam))
	{
	case CBN_SELCHANGE:
		OnCbnSelChange();
		break;
	}

	switch(LOWORD(wParam))
	{
	case IDC_DEFAULTCOLUMNS_MOVEUP:
		OnMoveColumn(TRUE);
		break;

	case IDC_DEFAULTCOLUMNS_MOVEDOWN:
		OnMoveColumn(FALSE);
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

INT_PTR SetDefaultColumnsDialog::OnNotify(NMHDR *pnmhdr)
{
	switch(pnmhdr->code)
	{
	case LVN_ITEMCHANGED:
		OnLvnItemChanged(reinterpret_cast<NMLISTVIEW *>(pnmhdr));
		break;
	}

	return 0;
}

INT_PTR SetDefaultColumnsDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

void SetDefaultColumnsDialog::SaveState()
{
	m_psdcdps->SaveDialogPosition(m_hDlg);

	int iSelected = static_cast<int>(SendDlgItemMessage(m_hDlg,IDC_DEFAULTCOLUMNS_COMBOBOX,CB_GETCURSEL,0,0));
	auto itr = m_FolderMap.find(iSelected);
	m_psdcdps->m_FolderType = itr->second;

	m_psdcdps->m_bStateSaved = TRUE;
}

void SetDefaultColumnsDialog::OnOk()
{
	HWND hComboBox = GetDlgItem(m_hDlg,IDC_DEFAULTCOLUMNS_COMBOBOX);
	int iSelected = static_cast<int>(SendMessage(hComboBox,CB_GETCURSEL,0,0));

	auto itr = m_FolderMap.find(iSelected);

	SaveCurrentColumnState(itr->second);

	EndDialog(m_hDlg,1);
}

void SetDefaultColumnsDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

void SetDefaultColumnsDialog::OnCbnSelChange()
{
	HWND hComboBox = GetDlgItem(m_hDlg,IDC_DEFAULTCOLUMNS_COMBOBOX);
	int iSelected = static_cast<int>(SendMessage(hComboBox,CB_GETCURSEL,0,0));

	auto itr = m_FolderMap.find(iSelected);

	/* Save the current column set, then setup the columns
	for the folder type that was selected. */
	SaveCurrentColumnState(m_PreviousFolderType);
	SetupFolderColumns(itr->second);

	m_PreviousFolderType = itr->second;
}

void SetDefaultColumnsDialog::SaveCurrentColumnState(FolderType_t FolderType)
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_DEFAULTCOLUMNS_LISTVIEW);

	auto currentColumns = GetCurrentColumnList(FolderType);
	std::vector<Column_t> tempColumns;

	for(int i = 0;i < ListView_GetItemCount(hListView);i++)
	{
		LVITEM lvItem;
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= i;
		lvItem.iSubItem	= 0;
		ListView_GetItem(hListView,&lvItem);

		/* Since the column list will be rebuilt, find this column
		in the current list, and reuse its width. */
		UINT id = static_cast<int>(lvItem.lParam);
		auto itr = std::find_if(currentColumns.begin(),currentColumns.end(),
			[id](const Column_t &Column){return Column.id == id;});

		Column_t Column;
		Column.id		= id;
		Column.iWidth	= itr->iWidth;
		Column.bChecked	= ListView_GetCheckState(hListView,i);
		tempColumns.push_back(Column);
	}

	currentColumns = tempColumns;
}

void SetDefaultColumnsDialog::SetupFolderColumns(FolderType_t FolderType)
{
	auto columns = GetCurrentColumnList(FolderType);

	HWND hListView = GetDlgItem(m_hDlg,IDC_DEFAULTCOLUMNS_LISTVIEW);
	ListView_DeleteAllItems(hListView);

	int iItem = 0;

	for(const auto &column : columns)
	{
		TCHAR szText[64];
		LoadString(GetInstance(),ShellBrowser::LookupColumnNameStringIndex(column.id),
			szText,SIZEOF_ARRAY(szText));

		LVITEM lvItem;
		lvItem.mask		= LVIF_TEXT | LVIF_PARAM;
		lvItem.iItem	= iItem;
		lvItem.iSubItem	= 0;
		lvItem.pszText	= szText;
		lvItem.lParam	= column.id;
		ListView_InsertItem(hListView,&lvItem);

		ListView_SetCheckState(hListView,iItem,column.bChecked);

		iItem++;
	}

	NListView::ListView_SelectItem(hListView,0,TRUE);
}

std::vector<Column_t> &SetDefaultColumnsDialog::GetCurrentColumnList(FolderType_t FolderType)
{
	switch(FolderType)
	{
	case FOLDER_TYPE_GENERAL:
		return m_folderColumns.realFolderColumns;

	case FOLDER_TYPE_COMPUTER:
		return m_folderColumns.myComputerColumns;

	case FOLDER_TYPE_CONTROL_PANEL:
		return m_folderColumns.controlPanelColumns;

	case FOLDER_TYPE_NETWORK:
		return m_folderColumns.networkConnectionsColumns;

	case FOLDER_TYPE_NETWORK_PLACES:
		return m_folderColumns.myNetworkPlacesColumns;

	case FOLDER_TYPE_PRINTERS:
		return m_folderColumns.printersColumns;

	case FOLDER_TYPE_RECYCLE_BIN:
		return m_folderColumns.recycleBinColumns;
	}

	throw std::runtime_error("Unknown folder type selected");
}

void SetDefaultColumnsDialog::OnLvnItemChanged(NMLISTVIEW *pnmlv)
{
	if(pnmlv->uNewState & LVIS_SELECTED)
	{
		HWND hListView = GetDlgItem(m_hDlg,IDC_COLUMNS_LISTVIEW);

		LVITEM lvItem;
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= pnmlv->iItem;
		lvItem.iSubItem	= 0;
		ListView_GetItem(hListView,&lvItem);

		int iDescriptionStringIndex = ShellBrowser::LookupColumnDescriptionStringIndex(static_cast<int>(lvItem.lParam));

		TCHAR szColumnDescription[128];
		LoadString(GetInstance(),iDescriptionStringIndex,szColumnDescription,
			SIZEOF_ARRAY(szColumnDescription));
		SetDlgItemText(m_hDlg,IDC_COLUMNS_DESCRIPTION,szColumnDescription);
	}
}

void SetDefaultColumnsDialog::OnMoveColumn(bool bUp)
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

		SetFocus(hListView);
	}
}

SetDefaultColumnsDialogPersistentSettings::SetDefaultColumnsDialogPersistentSettings() :
DialogSettings(SETTINGS_KEY)
{
	m_FolderType = FOLDER_TYPE_GENERAL;
}

SetDefaultColumnsDialogPersistentSettings& SetDefaultColumnsDialogPersistentSettings::GetInstance()
{
	static SetDefaultColumnsDialogPersistentSettings mfdps;
	return mfdps;
}

void SetDefaultColumnsDialogPersistentSettings::SaveExtraRegistrySettings(HKEY hKey)
{
	NRegistrySettings::SaveDwordToRegistry(hKey, SETTING_FOLDER_TYPE, m_FolderType);
}

void SetDefaultColumnsDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	NRegistrySettings::ReadDwordFromRegistry(hKey, SETTING_FOLDER_TYPE, reinterpret_cast<DWORD *>(&m_FolderType));
}

void SetDefaultColumnsDialogPersistentSettings::SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom,
	IXMLDOMElement *pParentNode)
{
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_FOLDER_TYPE, NXMLSettings::EncodeIntValue(m_FolderType));
}

void SetDefaultColumnsDialogPersistentSettings::LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue)
{
	if(lstrcmpi(bstrName, SETTING_FOLDER_TYPE) == 0)
	{
		m_FolderType = static_cast<FolderType_t>(NXMLSettings::DecodeIntValue(bstrValue));
	}
}