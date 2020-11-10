// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SetDefaultColumnsDialog.h"
#include "DarkModeHelper.h"
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

SetDefaultColumnsDialog::SetDefaultColumnsDialog(
	HINSTANCE hInstance, HWND hParent, FolderColumns &folderColumns) :
	DarkModeDialogBase(hInstance, IDD_SETDEFAULTCOLUMNS, hParent, true),
	m_folderColumns(folderColumns)
{
	m_psdcdps = &SetDefaultColumnsDialogPersistentSettings::GetInstance();
}

INT_PTR SetDefaultColumnsDialog::OnInitDialog()
{
	m_icon.reset(LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_MAIN)));
	SetClassLongPtr(m_hDlg, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(m_icon.get()));

	HWND hComboBox = GetDlgItem(m_hDlg, IDC_DEFAULTCOLUMNS_COMBOBOX);

	TCHAR szFolderName[MAX_PATH];
	int iPos;

	GetCsidlDisplayName(CSIDL_CONTROLS, szFolderName, SIZEOF_ARRAY(szFolderName), SHGDN_INFOLDER);
	iPos = static_cast<int>(SendMessage(hComboBox, CB_INSERTSTRING, static_cast<WPARAM>(-1),
		reinterpret_cast<LPARAM>(szFolderName)));
	m_FolderMap.insert(
		std::unordered_map<int, FolderType>::value_type(iPos, FolderType::ControlPanel));

	LoadString(GetInstance(), IDS_DEFAULTCOLUMNS_GENERAL, szFolderName, SIZEOF_ARRAY(szFolderName));
	iPos = static_cast<int>(SendMessage(hComboBox, CB_INSERTSTRING, static_cast<WPARAM>(-1),
		reinterpret_cast<LPARAM>(szFolderName)));
	m_FolderMap.insert(std::unordered_map<int, FolderType>::value_type(iPos, FolderType::General));

	GetCsidlDisplayName(CSIDL_DRIVES, szFolderName, SIZEOF_ARRAY(szFolderName), SHGDN_INFOLDER);
	iPos = static_cast<int>(SendMessage(hComboBox, CB_INSERTSTRING, static_cast<WPARAM>(-1),
		reinterpret_cast<LPARAM>(szFolderName)));
	m_FolderMap.insert(std::unordered_map<int, FolderType>::value_type(iPos, FolderType::Computer));

	GetCsidlDisplayName(
		CSIDL_CONNECTIONS, szFolderName, SIZEOF_ARRAY(szFolderName), SHGDN_INFOLDER);
	iPos = static_cast<int>(SendMessage(hComboBox, CB_INSERTSTRING, static_cast<WPARAM>(-1),
		reinterpret_cast<LPARAM>(szFolderName)));
	m_FolderMap.insert(std::unordered_map<int, FolderType>::value_type(iPos, FolderType::Network));

	GetCsidlDisplayName(CSIDL_NETWORK, szFolderName, SIZEOF_ARRAY(szFolderName), SHGDN_INFOLDER);
	iPos = static_cast<int>(SendMessage(hComboBox, CB_INSERTSTRING, static_cast<WPARAM>(-1),
		reinterpret_cast<LPARAM>(szFolderName)));
	m_FolderMap.insert(
		std::unordered_map<int, FolderType>::value_type(iPos, FolderType::NetworkPlaces));

	GetCsidlDisplayName(CSIDL_PRINTERS, szFolderName, SIZEOF_ARRAY(szFolderName), SHGDN_INFOLDER);
	iPos = static_cast<int>(SendMessage(hComboBox, CB_INSERTSTRING, static_cast<WPARAM>(-1),
		reinterpret_cast<LPARAM>(szFolderName)));
	m_FolderMap.insert(std::unordered_map<int, FolderType>::value_type(iPos, FolderType::Printers));

	GetCsidlDisplayName(CSIDL_BITBUCKET, szFolderName, SIZEOF_ARRAY(szFolderName), SHGDN_INFOLDER);
	iPos = static_cast<int>(SendMessage(hComboBox, CB_INSERTSTRING, static_cast<WPARAM>(-1),
		reinterpret_cast<LPARAM>(szFolderName)));
	m_FolderMap.insert(
		std::unordered_map<int, FolderType>::value_type(iPos, FolderType::RecycleBin));

	auto folderType = m_psdcdps->m_FolderType;
	auto itr = std::find_if(m_FolderMap.begin(), m_FolderMap.end(),
		[folderType](const std::unordered_map<int, FolderType>::value_type &vt) {
			return vt.second == folderType;
		});
	SendMessage(hComboBox, CB_SETCURSEL, itr->first, 0);

	m_PreviousFolderType = m_psdcdps->m_FolderType;

	HWND hListView = GetDlgItem(m_hDlg, IDC_DEFAULTCOLUMNS_LISTVIEW);
	SetWindowTheme(hListView, L"Explorer", nullptr);

	ListView_SetExtendedListViewStyleEx(hListView, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);

	LVCOLUMN lvColumn;
	lvColumn.mask = LVCF_WIDTH;
	lvColumn.cx = 180;
	ListView_InsertColumn(hListView, 0, &lvColumn);

	SetupFolderColumns(m_psdcdps->m_FolderType);

	SetFocus(hListView);

	AllowDarkModeForControls({ IDC_DEFAULTCOLUMNS_MOVEUP, IDC_DEFAULTCOLUMNS_MOVEDOWN });
	AllowDarkModeForComboBoxes({ IDC_DEFAULTCOLUMNS_COMBOBOX });

	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (darkModeHelper.IsDarkModeEnabled())
	{
		darkModeHelper.SetListViewDarkModeColors(hListView);
	}

	m_psdcdps->RestoreDialogPosition(m_hDlg, true);

	return 0;
}

void SetDefaultColumnsDialog::GetResizableControlInformation(
	BaseDialog::DialogSizeConstraint &dsc, std::list<ResizableDialog::Control> &ControlList)
{
	dsc = BaseDialog::DialogSizeConstraint::None;

	ResizableDialog::Control control;

	control.iID = IDC_COLUMNS_LISTVIEW;
	control.Type = ResizableDialog::ControlType::Resize;
	control.Constraint = ResizableDialog::ControlConstraint::None;
	ControlList.push_back(control);

	control.iID = IDC_COLUMNS_MOVEUP;
	control.Type = ResizableDialog::ControlType::Move;
	control.Constraint = ResizableDialog::ControlConstraint::X;
	ControlList.push_back(control);

	control.iID = IDC_COLUMNS_MOVEDOWN;
	control.Type = ResizableDialog::ControlType::Move;
	control.Constraint = ResizableDialog::ControlConstraint::X;
	ControlList.push_back(control);

	control.iID = IDC_STATIC_DESCRIPTION;
	control.Type = ResizableDialog::ControlType::Move;
	control.Constraint = ResizableDialog::ControlConstraint::Y;
	ControlList.push_back(control);

	control.iID = IDC_DEFAULTCOLUMNS_DESCRIPTION;
	control.Type = ResizableDialog::ControlType::Move;
	control.Constraint = ResizableDialog::ControlConstraint::Y;
	ControlList.push_back(control);

	control.iID = IDC_DEFAULTCOLUMNS_DESCRIPTION;
	control.Type = ResizableDialog::ControlType::Resize;
	control.Constraint = ResizableDialog::ControlConstraint::X;
	ControlList.push_back(control);

	control.iID = IDC_STATIC_ETCHEDHORZ;
	control.Type = ResizableDialog::ControlType::Move;
	control.Constraint = ResizableDialog::ControlConstraint::Y;
	ControlList.push_back(control);

	control.iID = IDC_STATIC_ETCHEDHORZ;
	control.Type = ResizableDialog::ControlType::Resize;
	control.Constraint = ResizableDialog::ControlConstraint::X;
	ControlList.push_back(control);

	control.iID = IDOK;
	control.Type = ResizableDialog::ControlType::Move;
	control.Constraint = ResizableDialog::ControlConstraint::None;
	ControlList.push_back(control);

	control.iID = IDCANCEL;
	control.Type = ResizableDialog::ControlType::Move;
	control.Constraint = ResizableDialog::ControlConstraint::None;
	ControlList.push_back(control);
}

INT_PTR SetDefaultColumnsDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (HIWORD(wParam))
	{
	case CBN_SELCHANGE:
		OnCbnSelChange();
		break;
	}

	switch (LOWORD(wParam))
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
	switch (pnmhdr->code)
	{
	case LVN_ITEMCHANGED:
		OnLvnItemChanged(reinterpret_cast<NMLISTVIEW *>(pnmhdr));
		break;
	}

	return 0;
}

INT_PTR SetDefaultColumnsDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
	return 0;
}

void SetDefaultColumnsDialog::SaveState()
{
	m_psdcdps->SaveDialogPosition(m_hDlg);

	int iSelected = static_cast<int>(
		SendDlgItemMessage(m_hDlg, IDC_DEFAULTCOLUMNS_COMBOBOX, CB_GETCURSEL, 0, 0));
	auto itr = m_FolderMap.find(iSelected);
	m_psdcdps->m_FolderType = itr->second;

	m_psdcdps->m_bStateSaved = TRUE;
}

void SetDefaultColumnsDialog::OnOk()
{
	HWND hComboBox = GetDlgItem(m_hDlg, IDC_DEFAULTCOLUMNS_COMBOBOX);
	int iSelected = static_cast<int>(SendMessage(hComboBox, CB_GETCURSEL, 0, 0));

	auto itr = m_FolderMap.find(iSelected);

	SaveCurrentColumnState(itr->second);

	EndDialog(m_hDlg, 1);
}

void SetDefaultColumnsDialog::OnCancel()
{
	EndDialog(m_hDlg, 0);
}

void SetDefaultColumnsDialog::OnCbnSelChange()
{
	HWND hComboBox = GetDlgItem(m_hDlg, IDC_DEFAULTCOLUMNS_COMBOBOX);
	int iSelected = static_cast<int>(SendMessage(hComboBox, CB_GETCURSEL, 0, 0));

	auto itr = m_FolderMap.find(iSelected);

	/* Save the current column set, then setup the columns
	for the folder type that was selected. */
	SaveCurrentColumnState(m_PreviousFolderType);
	SetupFolderColumns(itr->second);

	m_PreviousFolderType = itr->second;
}

void SetDefaultColumnsDialog::SaveCurrentColumnState(FolderType folderType)
{
	HWND hListView = GetDlgItem(m_hDlg, IDC_DEFAULTCOLUMNS_LISTVIEW);

	auto & currentColumns = GetCurrentColumnList(folderType);
	std::vector<Column_t> tempColumns;

	for (int i = 0; i < ListView_GetItemCount(hListView); i++)
	{
		LVITEM lvItem;
		lvItem.mask = LVIF_PARAM;
		lvItem.iItem = i;
		lvItem.iSubItem = 0;
		ListView_GetItem(hListView, &lvItem);

		/* Since the column list will be rebuilt, find this column
		in the current list, and reuse its width. */
		ColumnType columnType = static_cast<ColumnType>(lvItem.lParam);
		auto itr = std::find_if(
			currentColumns.begin(), currentColumns.end(), [columnType](const Column_t &column) {
				return column.type == columnType;
			});

		Column_t column;
		column.type = columnType;
		column.iWidth = itr->iWidth;
		column.bChecked = ListView_GetCheckState(hListView, i);
		tempColumns.push_back(column);
	}

	currentColumns.swap(tempColumns);
}

void SetDefaultColumnsDialog::SetupFolderColumns(FolderType folderType)
{
	auto columns = GetCurrentColumnList(folderType);

	HWND hListView = GetDlgItem(m_hDlg, IDC_DEFAULTCOLUMNS_LISTVIEW);
	ListView_DeleteAllItems(hListView);

	int iItem = 0;

	for (const auto &column : columns)
	{
		TCHAR szText[64];
		LoadString(GetInstance(), ShellBrowser::LookupColumnNameStringIndex(column.type), szText,
			SIZEOF_ARRAY(szText));

		LVITEM lvItem;
		lvItem.mask = LVIF_TEXT | LVIF_PARAM;
		lvItem.iItem = iItem;
		lvItem.iSubItem = 0;
		lvItem.pszText = szText;
		lvItem.lParam = static_cast<LPARAM>(column.type);
		ListView_InsertItem(hListView, &lvItem);

		ListView_SetCheckState(hListView, iItem, column.bChecked);

		iItem++;
	}

	ListViewHelper::SelectItem(hListView, 0, TRUE);
}

std::vector<Column_t> &SetDefaultColumnsDialog::GetCurrentColumnList(FolderType folderType)
{
	switch (folderType)
	{
	case FolderType::General:
		return m_folderColumns.realFolderColumns;

	case FolderType::Computer:
		return m_folderColumns.myComputerColumns;

	case FolderType::ControlPanel:
		return m_folderColumns.controlPanelColumns;

	case FolderType::Network:
		return m_folderColumns.networkConnectionsColumns;

	case FolderType::NetworkPlaces:
		return m_folderColumns.myNetworkPlacesColumns;

	case FolderType::Printers:
		return m_folderColumns.printersColumns;

	case FolderType::RecycleBin:
		return m_folderColumns.recycleBinColumns;
	}

	throw std::runtime_error("Unknown folder type selected");
}

void SetDefaultColumnsDialog::OnLvnItemChanged(NMLISTVIEW *pnmlv)
{
	if (pnmlv->uNewState & LVIS_SELECTED)
	{
		HWND hListView = GetDlgItem(m_hDlg, IDC_COLUMNS_LISTVIEW);

		LVITEM lvItem;
		lvItem.mask = LVIF_PARAM;
		lvItem.iItem = pnmlv->iItem;
		lvItem.iSubItem = 0;
		ListView_GetItem(hListView, &lvItem);

		int iDescriptionStringIndex =
			ShellBrowser::LookupColumnDescriptionStringIndex(static_cast<ColumnType>(lvItem.lParam));

		TCHAR szColumnDescription[128];
		LoadString(GetInstance(), iDescriptionStringIndex, szColumnDescription,
			SIZEOF_ARRAY(szColumnDescription));
		SetDlgItemText(m_hDlg, IDC_COLUMNS_DESCRIPTION, szColumnDescription);
	}
}

void SetDefaultColumnsDialog::OnMoveColumn(bool bUp)
{
	HWND hListView = GetDlgItem(m_hDlg, IDC_COLUMNS_LISTVIEW);

	int iSelected = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);

	if (iSelected != -1)
	{
		if (bUp)
		{
			ListViewHelper::SwapItems(hListView, iSelected, iSelected - 1, TRUE);
		}
		else
		{
			ListViewHelper::SwapItems(hListView, iSelected, iSelected + 1, TRUE);
		}

		SetFocus(hListView);
	}
}

SetDefaultColumnsDialogPersistentSettings::SetDefaultColumnsDialogPersistentSettings() :
	DialogSettings(SETTINGS_KEY)
{
	m_FolderType = FolderType::General;
}

SetDefaultColumnsDialogPersistentSettings &SetDefaultColumnsDialogPersistentSettings::GetInstance()
{
	static SetDefaultColumnsDialogPersistentSettings mfdps;
	return mfdps;
}

void SetDefaultColumnsDialogPersistentSettings::SaveExtraRegistrySettings(HKEY hKey)
{
	NRegistrySettings::SaveDwordToRegistry(
		hKey, SETTING_FOLDER_TYPE, static_cast<DWORD>(m_FolderType));
}

void SetDefaultColumnsDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	DWORD value;
	NRegistrySettings::ReadDwordFromRegistry(hKey, SETTING_FOLDER_TYPE, &value);
	m_FolderType = static_cast<FolderType>(value);
}

void SetDefaultColumnsDialogPersistentSettings::SaveExtraXMLSettings(
	IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode)
{
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_FOLDER_TYPE,
		NXMLSettings::EncodeIntValue(static_cast<int>(m_FolderType)));
}

void SetDefaultColumnsDialogPersistentSettings::LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue)
{
	if (lstrcmpi(bstrName, SETTING_FOLDER_TYPE) == 0)
	{
		m_FolderType = static_cast<FolderType>(NXMLSettings::DecodeIntValue(bstrValue));
	}
}