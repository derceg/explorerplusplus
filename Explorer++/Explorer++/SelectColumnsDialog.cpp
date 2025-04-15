// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SelectColumnsDialog.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "ShellBrowser/ColumnHelper.h"
#include "ShellBrowser/Columns.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "../Helper/ListViewHelper.h"
#include <algorithm>
#include <functional>

const TCHAR SelectColumnsDialogPersistentSettings::SETTINGS_KEY[] = _T("SelectColumns");

SelectColumnsDialog::SelectColumnsDialog(const ResourceLoader *resourceLoader,
	HINSTANCE resourceInstance, HWND hParent, ShellBrowserImpl *shellBrowser) :
	BaseDialog(resourceLoader, resourceInstance, IDD_SELECTCOLUMNS, hParent,
		DialogSizingType::Both),
	m_shellBrowser(shellBrowser),
	m_bColumnsSwapped(FALSE)
{
	m_persistentSettings = &SelectColumnsDialogPersistentSettings::GetInstance();
}

INT_PTR SelectColumnsDialog::OnInitDialog()
{
	HWND hListView = GetDlgItem(m_hDlg, IDC_COLUMNS_LISTVIEW);
	ListView_SetExtendedListViewStyleEx(hListView, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);

	LVCOLUMN lvColumn;
	lvColumn.mask = 0;
	ListView_InsertColumn(hListView, 0, &lvColumn);

	auto currentColumns = m_shellBrowser->GetCurrentColumns();

	std::sort(currentColumns.begin(), currentColumns.end(),
		std::bind_front(&SelectColumnsDialog::CompareColumns, this));

	int iItem = 0;

	for (const auto &column : currentColumns)
	{
		std::wstring name = GetColumnName(m_resourceLoader, column.type);

		LVITEM lvItem;
		lvItem.mask = LVIF_TEXT | LVIF_PARAM;
		lvItem.iItem = iItem;
		lvItem.iSubItem = 0;
		lvItem.pszText = name.data();
		lvItem.lParam = static_cast<LPARAM>(column.type);
		ListView_InsertItem(hListView, &lvItem);

		ListView_SetCheckState(hListView, iItem, column.checked);

		iItem++;
	}

	ListView_SetColumnWidth(hListView, 0, LVSCW_AUTOSIZE);

	ListViewHelper::SelectItem(hListView, 0, true);
	SetFocus(hListView);

	m_persistentSettings->RestoreDialogPosition(m_hDlg, true);

	return 0;
}

wil::unique_hicon SelectColumnsDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return m_resourceLoader->LoadIconFromPNGAndScale(Icon::SelectColumns, iconWidth, iconHeight);
}

bool SelectColumnsDialog::CompareColumns(const Column_t &column1, const Column_t &column2)
{
	if (column1.checked && column2.checked)
	{
		// If both column are checked, preserve the input ordering (this is
		// the order that the columns will actually appear in the listview).
		// This matches the behavior of Windows Explorer.
		return false;
	}
	else if (column1.checked && !column2.checked)
	{
		return true;
	}
	else if (!column1.checked && column2.checked)
	{
		return false;
	}

	auto column1Name = GetColumnName(m_resourceLoader, column1.type);
	auto column2Name = GetColumnName(m_resourceLoader, column2.type);

	int ret = StrCmpLogicalW(column1Name.c_str(), column2Name.c_str());

	if (ret == -1)
	{
		return true;
	}
	else
	{
		return false;
	}
}

std::vector<ResizableDialogControl> SelectColumnsDialog::GetResizableControls()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_COLUMNS_LISTVIEW), MovingType::None,
		SizingType::Both);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_COLUMNS_MOVEUP), MovingType::Horizontal,
		SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_COLUMNS_MOVEDOWN), MovingType::Horizontal,
		SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_STATIC_DESCRIPTION), MovingType::Vertical,
		SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_COLUMNS_DESCRIPTION), MovingType::Vertical,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_STATIC_ETCHEDHORZ), MovingType::Vertical,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(m_hDlg, IDOK), MovingType::Both, SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDCANCEL), MovingType::Both, SizingType::None);
	return controls;
}

INT_PTR SelectColumnsDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam))
	{
	case IDC_COLUMNS_MOVEUP:
		OnMoveColumn(MoveDirection::Up);
		break;

	case IDC_COLUMNS_MOVEDOWN:
		OnMoveColumn(MoveDirection::Down);
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

INT_PTR SelectColumnsDialog::OnNotify(NMHDR *pnmhdr)
{
	switch (pnmhdr->code)
	{
	case LVN_ITEMCHANGING:
	{
		BOOL res = OnLvnItemChanging(reinterpret_cast<NMLISTVIEW *>(pnmhdr));
		SetWindowLongPtr(m_hDlg, DWLP_MSGRESULT, res);
		return TRUE;
	}

	case LVN_ITEMCHANGED:
		OnLvnItemChanged(reinterpret_cast<NMLISTVIEW *>(pnmhdr));
		break;
	}

	return 0;
}

INT_PTR SelectColumnsDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
	return 0;
}

void SelectColumnsDialog::SaveState()
{
	m_persistentSettings->SaveDialogPosition(m_hDlg);
	m_persistentSettings->m_bStateSaved = TRUE;
}

void SelectColumnsDialog::OnOk()
{
	HWND hListView = GetDlgItem(m_hDlg, IDC_COLUMNS_LISTVIEW);
	std::vector<Column_t> updatedColumns;

	auto currentColumns = m_shellBrowser->GetCurrentColumns();

	for (int i = 0; i < ListView_GetItemCount(hListView); i++)
	{
		LVITEM lvItem;
		lvItem.mask = LVIF_PARAM;
		lvItem.iItem = i;
		lvItem.iSubItem = 0;
		ListView_GetItem(hListView, &lvItem);

		auto columnType =
			ColumnType::_from_integral_nothrow(static_cast<ColumnType::_integral>(lvItem.lParam));
		CHECK(columnType);
		auto itr = std::find_if(currentColumns.begin(), currentColumns.end(),
			[&columnType](const Column_t &column) { return column.type == *columnType; });

		Column_t column;
		column.type = *columnType;
		column.width = itr->width;
		column.checked = ListView_GetCheckState(hListView, i);
		updatedColumns.push_back(column);
	}

	m_shellBrowser->SetCurrentColumns(updatedColumns);

	if (m_bColumnsSwapped)
	{
		m_shellBrowser->GetNavigationController()->Refresh();
	}

	EndDialog(m_hDlg, 1);
}

void SelectColumnsDialog::OnCancel()
{
	EndDialog(m_hDlg, 0);
}

BOOL SelectColumnsDialog::OnLvnItemChanging(const NMLISTVIEW *nmlv)
{
	if (nmlv->uChanged != LVIF_STATE || (nmlv->uNewState & LVIS_STATEIMAGEMASK) == 0)
	{
		return FALSE;
	}

	BOOL checked = ((nmlv->uNewState & LVIS_STATEIMAGEMASK) >> 12) == 2;

	if (checked)
	{
		return FALSE;
	}

	HWND listView = GetDlgItem(m_hDlg, IDC_COLUMNS_LISTVIEW);
	int numItems = ListView_GetItemCount(listView);
	bool anyOtherItemsChecked = false;

	for (int i = 0; i < numItems; i++)
	{
		if (i == nmlv->iItem)
		{
			continue;
		}

		if (ListView_GetCheckState(listView, i))
		{
			anyOtherItemsChecked = true;
			break;
		}
	}

	if (!anyOtherItemsChecked)
	{
		// There should always be at least one column that's checked.
		return TRUE;
	}

	return FALSE;
}

void SelectColumnsDialog::OnLvnItemChanged(const NMLISTVIEW *pnmlv)
{
	if (pnmlv->uNewState & LVIS_SELECTED)
	{
		HWND hListView = GetDlgItem(m_hDlg, IDC_COLUMNS_LISTVIEW);

		LVITEM lvItem;
		lvItem.mask = LVIF_PARAM;
		lvItem.iItem = pnmlv->iItem;
		lvItem.iSubItem = 0;
		ListView_GetItem(hListView, &lvItem);

		auto columnType =
			ColumnType::_from_integral_nothrow(static_cast<ColumnType::_integral>(lvItem.lParam));
		CHECK(columnType);

		auto columnDescription = GetColumnDescription(m_resourceLoader, *columnType);
		SetDlgItemText(m_hDlg, IDC_COLUMNS_DESCRIPTION, columnDescription.c_str());
	}
}

void SelectColumnsDialog::OnMoveColumn(MoveDirection direction)
{
	HWND listView = GetDlgItem(m_hDlg, IDC_COLUMNS_LISTVIEW);

	int selectedItemIndex = ListView_GetNextItem(listView, -1, LVNI_SELECTED);

	if (selectedItemIndex == -1)
	{
		return;
	}

	int newIndex;

	if (direction == MoveDirection::Up)
	{
		newIndex = selectedItemIndex - 1;
	}
	else
	{
		newIndex = selectedItemIndex + 1;
	}

	if (newIndex < 0 || newIndex >= ListView_GetItemCount(listView))
	{
		return;
	}

	ListViewHelper::SwapItems(listView, selectedItemIndex, newIndex);
	ListView_EnsureVisible(listView, newIndex, false);

	m_bColumnsSwapped = TRUE;

	SetFocus(listView);
}

SelectColumnsDialogPersistentSettings::SelectColumnsDialogPersistentSettings() :
	DialogSettings(SETTINGS_KEY)
{
}

SelectColumnsDialogPersistentSettings &SelectColumnsDialogPersistentSettings::GetInstance()
{
	static SelectColumnsDialogPersistentSettings mfdps;
	return mfdps;
}
