// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SearchTabsDialog.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "SearchTabsModel.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "Tab.h"
#include "TabContainer.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/ScopedRedrawDisabler.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/WindowSubclass.h"
#include <glog/logging.h>

SearchTabsDialog *SearchTabsDialog::Create(HWND parent, std::unique_ptr<SearchTabsModel> model,
	const ResourceLoader *resourceLoader)
{
	return new SearchTabsDialog(parent, std::move(model), resourceLoader);
}

SearchTabsDialog::SearchTabsDialog(HWND parent, std::unique_ptr<SearchTabsModel> model,
	const ResourceLoader *resourceLoader) :
	BaseDialog(resourceLoader, IDD_SEARCH_TABS, parent, BaseDialog::DialogSizingType::Both),
	m_model(std::move(model)),
	m_persistentSettings(&SearchTabsDialogPersistentSettings::GetInstance())
{
	m_model->SetSearchTerm(m_persistentSettings->m_searchTerm);
}

INT_PTR SearchTabsDialog::OnInitDialog()
{
	SetupListView();
	SetupEditControl();

	m_model->updatedSignal.AddObserver(std::bind_front(&SearchTabsDialog::RefreshTabList, this));

	SendMessage(m_hDlg, WM_NEXTDLGCTL,
		reinterpret_cast<WPARAM>(GetDlgItem(m_hDlg, IDC_SEARCH_TABS_SEARCH_TERM)), true);

	m_persistentSettings->RestoreDialogPosition(m_hDlg, true);

	return FALSE;
}

wil::unique_hicon SearchTabsDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	UNREFERENCED_PARAMETER(iconWidth);
	UNREFERENCED_PARAMETER(iconHeight);

	return wil::unique_hicon(LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_MAIN)));
}

std::vector<ResizableDialogControl> SearchTabsDialog::GetResizableControls()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_SEARCH_TABS_TAB_LIST), MovingType::None,
		SizingType::Both);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_SEARCH_TABS_SEARCH_TERM), MovingType::Vertical,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(m_hDlg, IDOK), MovingType::Both, SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDCANCEL), MovingType::Both, SizingType::None);
	return controls;
}

void SearchTabsDialog::SetupListView()
{
	HWND listView = GetDlgItem(m_hDlg, IDC_SEARCH_TABS_TAB_LIST);
	ListView_SetExtendedListViewStyle(listView,
		LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	InsertColumns();
	RefreshTabList();
}

void SearchTabsDialog::InsertColumns()
{
	int index = 0;

	for (auto column : COLUMNS)
	{
		InsertColumn(column, index);
		index++;
	}
}

void SearchTabsDialog::InsertColumn(const Column &column, int index)
{
	std::wstring columnText = GetColumnText(column.type);

	RECT listViewRect;
	HWND listView = GetDlgItem(m_hDlg, IDC_SEARCH_TABS_TAB_LIST);
	auto res = GetClientRect(listView, &listViewRect);
	CHECK(res);

	LVCOLUMN lvColumn = {};
	lvColumn.mask = LVCF_TEXT | LVCF_WIDTH;
	lvColumn.pszText = columnText.data();
	lvColumn.cx = static_cast<int>(column.percentageWidth * GetRectWidth(&listViewRect));
	int insertedIndex = ListView_InsertColumn(listView, index, &lvColumn);
	CHECK(insertedIndex == index);
}

std::wstring SearchTabsDialog::GetColumnText(ColumnType columnType)
{
	UINT stringId;

	switch (columnType)
	{
	case SearchTabsDialog::ColumnType::TabName:
		stringId = IDS_SEARCH_TABS_COLUMN_TAB_NAME;
		break;

	case SearchTabsDialog::ColumnType::Path:
		stringId = IDS_SEARCH_TABS_COLUMN_PATH;
		break;

	default:
		LOG(FATAL) << "Search tabs column type not found";
		__assume(0);
	}

	return m_resourceLoader->LoadString(stringId);
}

void SearchTabsDialog::RefreshTabList()
{
	HWND listView = GetDlgItem(m_hDlg, IDC_SEARCH_TABS_TAB_LIST);

	ScopedRedrawDisabler redrawDisabler(listView);
	ListView_DeleteAllItems(listView);
	AddTabs();
}

void SearchTabsDialog::AddTabs()
{
	HWND listView = GetDlgItem(m_hDlg, IDC_SEARCH_TABS_TAB_LIST);
	int index = 0;

	for (auto *tab : m_model->GetResults())
	{
		AddTab(tab, index);

		if (index == 0)
		{
			ListViewHelper::SelectItem(listView, index, true);
		}

		index++;
	}
}

void SearchTabsDialog::AddTab(const Tab *tab, int index)
{
	HWND listView = GetDlgItem(m_hDlg, IDC_SEARCH_TABS_TAB_LIST);

	LVITEM item = {};
	item.mask = LVIF_TEXT | LVIF_PARAM;
	item.iItem = index;
	item.iSubItem = 0;
	item.pszText = LPSTR_TEXTCALLBACK;
	item.lParam = reinterpret_cast<LPARAM>(tab);
	int finalIndex = ListView_InsertItem(listView, &item);
	CHECK(finalIndex == index);
}

void SearchTabsDialog::SetupEditControl()
{
	HWND edit = GetDlgItem(m_hDlg, IDC_SEARCH_TABS_SEARCH_TERM);

	m_editSubclass = std::make_unique<WindowSubclass>(edit,
		std::bind_front(&SearchTabsDialog::EditWndProc, this));

	auto placeHolderText =
		m_resourceLoader->LoadString(IDS_SEARCH_TABS_SEARCH_TERM_PLACEHOLDER_TEXT);
	SendMessage(edit, EM_SETCUEBANNER, true, reinterpret_cast<LPARAM>(placeHolderText.c_str()));

	SetWindowText(edit, m_model->GetSearchTerm().c_str());
}

INT_PTR SearchTabsDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (HIWORD(wParam) != 0)
	{
		switch (HIWORD(wParam))
		{
		case EN_CHANGE:
			m_model->SetSearchTerm(
				GetWindowString(GetDlgItem(m_hDlg, IDC_SEARCH_TABS_SEARCH_TERM)));
			break;
		}
	}
	else
	{
		switch (LOWORD(wParam))
		{
		case IDOK:
			OnOk();
			break;

		case IDCANCEL:
			OnCancel();
			break;
		}
	}

	return 0;
}

INT_PTR SearchTabsDialog::OnNotify(NMHDR *nmhdr)
{
	if (nmhdr->idFrom == IDC_SEARCH_TABS_TAB_LIST)
	{
		switch (nmhdr->code)
		{
		case NM_DBLCLK:
			OnListViewDoubleClick(reinterpret_cast<NMITEMACTIVATE *>(nmhdr));
			break;

		case LVN_GETDISPINFO:
			OnGetDispInfo(reinterpret_cast<NMLVDISPINFO *>(nmhdr));
			break;
		}
	}

	return 0;
}

void SearchTabsDialog::OnListViewDoubleClick(const NMITEMACTIVATE *itemActivate)
{
	if (itemActivate->iItem == -1)
	{
		return;
	}

	const Tab *tab = GetTabFromListView(itemActivate->iItem);
	tab->GetTabContainer()->SelectTab(*tab);

	DestroyWindow(m_hDlg);
}

void SearchTabsDialog::OnGetDispInfo(NMLVDISPINFO *dispInfo)
{
	if (WI_IsFlagSet(dispInfo->item.mask, LVIF_TEXT))
	{
		const Tab *tab = GetTabFromListView(dispInfo->item.iItem);

		CHECK(dispInfo->item.iSubItem >= 0 && dispInfo->item.iSubItem < std::ssize(COLUMNS));
		auto columnType = COLUMNS[dispInfo->item.iSubItem].type;

		auto text = GetTabColumnText(tab, columnType);
		StringCchCopy(dispInfo->item.pszText, dispInfo->item.cchTextMax, text.c_str());

		WI_SetFlag(dispInfo->item.mask, LVIF_DI_SETITEM);
	}
}

std::wstring SearchTabsDialog::GetTabColumnText(const Tab *tab, ColumnType columnType)
{
	switch (columnType)
	{
	case SearchTabsDialog::ColumnType::TabName:
		return tab->GetName();

	case SearchTabsDialog::ColumnType::Path:
		return tab->GetShellBrowserImpl()->GetDirectoryPath();

	default:
		LOG(FATAL) << "Search tabs column type not found";
	}
}

LRESULT SearchTabsDialog::EditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_UP:
			OnMoveListViewSelection(MoveDirection::Up);
			return 0;

		case VK_DOWN:
			OnMoveListViewSelection(MoveDirection::Down);
			return 0;
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void SearchTabsDialog::OnMoveListViewSelection(MoveDirection direction)
{
	HWND listView = GetDlgItem(m_hDlg, IDC_SEARCH_TABS_TAB_LIST);
	int currentItemIndex = ListView_GetNextItem(listView, -1, LVNI_SELECTED);

	if (currentItemIndex == -1)
	{
		currentItemIndex = ListView_GetNextItem(listView, -1, LVNI_FOCUSED);

		if (currentItemIndex == -1)
		{
			currentItemIndex = 0;
		}
	}

	int newIndex;

	if (direction == MoveDirection::Up)
	{
		newIndex = currentItemIndex - 1;
	}
	else
	{
		newIndex = currentItemIndex + 1;
	}

	if (newIndex < 0 || newIndex >= ListView_GetItemCount(listView))
	{
		return;
	}

	ListViewHelper::SelectItem(listView, newIndex, true);
}

void SearchTabsDialog::OnOk()
{
	HWND listView = GetDlgItem(m_hDlg, IDC_SEARCH_TABS_TAB_LIST);
	int selectedItemIndex = ListView_GetNextItem(listView, -1, LVNI_SELECTED);

	if (selectedItemIndex != -1)
	{
		const Tab *tab = GetTabFromListView(selectedItemIndex);
		tab->GetTabContainer()->SelectTab(*tab);
	}

	DestroyWindow(m_hDlg);
}

const Tab *SearchTabsDialog::GetTabFromListView(int index)
{
	LVITEM lvItem;
	lvItem.mask = LVIF_PARAM;
	lvItem.iItem = index;
	lvItem.iSubItem = 0;
	BOOL res = ListView_GetItem(GetDlgItem(m_hDlg, IDC_SEARCH_TABS_TAB_LIST), &lvItem);
	CHECK(res);

	return reinterpret_cast<const Tab *>(lvItem.lParam);
}

void SearchTabsDialog::OnCancel()
{
	DestroyWindow(m_hDlg);
}

INT_PTR SearchTabsDialog::OnClose()
{
	DestroyWindow(m_hDlg);
	return 0;
}

void SearchTabsDialog::SaveState()
{
	m_persistentSettings->SaveDialogPosition(m_hDlg);

	m_persistentSettings->m_searchTerm = m_model->GetSearchTerm();

	m_persistentSettings->m_bStateSaved = TRUE;
}

INT_PTR SearchTabsDialog::OnNcDestroy()
{
	delete this;

	return 0;
}

SearchTabsDialogPersistentSettings::SearchTabsDialogPersistentSettings() :
	DialogSettings(SETTINGS_KEY.c_str())
{
}

SearchTabsDialogPersistentSettings &SearchTabsDialogPersistentSettings::GetInstance()
{
	static SearchTabsDialogPersistentSettings persistentSettings;
	return persistentSettings;
}
