// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SearchTabsDialog.h"
#include "LabelEditHandler.h"
#include "ListView.h"
#include "MainResource.h"
#include "NoOpMenuHelpTextHost.h"
#include "ResourceLoader.h"
#include "SearchTabsListViewModel.h"
#include "SearchTabsModel.h"
#include "ShellBrowser/ShellBrowser.h"
#include "Tab.h"
#include "TabContainer.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/WindowSubclass.h"
#include <glog/logging.h>

SearchTabsDialog *SearchTabsDialog::Create(HWND parent, std::unique_ptr<SearchTabsModel> model,
	const TabList *tabList, AsyncIconFetcher *iconFetcher, const KeyboardState *keyboardState,
	const AcceleratorManager *acceleratorManager, const ResourceLoader *resourceLoader)
{
	return new SearchTabsDialog(parent, std::move(model), tabList, iconFetcher, keyboardState,
		acceleratorManager, resourceLoader);
}

SearchTabsDialog::SearchTabsDialog(HWND parent, std::unique_ptr<SearchTabsModel> model,
	const TabList *tabList, AsyncIconFetcher *iconFetcher, const KeyboardState *keyboardState,
	const AcceleratorManager *acceleratorManager, const ResourceLoader *resourceLoader) :
	BaseDialog(resourceLoader, IDD_SEARCH_TABS, parent, BaseDialog::DialogSizingType::Both),
	m_model(std::move(model)),
	m_tabList(tabList),
	m_iconFetcher(iconFetcher),
	m_keyboardState(keyboardState),
	m_acceleratorManager(acceleratorManager),
	m_persistentSettings(&SearchTabsDialogPersistentSettings::GetInstance())
{
	m_model->SetSearchTerm(m_persistentSettings->m_searchTerm);
}

INT_PTR SearchTabsDialog::OnInitDialog()
{
	SetupListView();
	SetupEditControl();

	SendMessage(m_hDlg, WM_NEXTDLGCTL,
		reinterpret_cast<WPARAM>(GetDlgItem(m_hDlg, IDC_SEARCH_TABS_SEARCH_TERM)), true);

	m_persistentSettings->RestoreDialogPosition(m_hDlg, true);

	m_listView->SizeLastColumnToFill();

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
	m_listView = std::make_unique<ListView>(GetDlgItem(m_hDlg, IDC_SEARCH_TABS_TAB_LIST),
		m_keyboardState, LabelEditHandler::CreateForDialog, NoOpMenuHelpTextHost::GetInstance(),
		m_acceleratorManager, m_resourceLoader);
	m_listView->AddExtendedStyles(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
	m_listView->SetupSmallShellImageList();

	m_listViewModel =
		std::make_unique<SearchTabsListViewModel>(m_model.get(), m_tabList, m_iconFetcher);

	m_connections.push_back(m_listViewModel->itemsRefreshedSignal.AddObserver(
		std::bind_front(&SearchTabsDialog::SelectFirstItem, this)));

	if (m_persistentSettings->m_sortColumn && m_persistentSettings->m_sortDirection)
	{
		m_listViewModel->SetSortDetails(*m_persistentSettings->m_sortColumn,
			*m_persistentSettings->m_sortDirection);
	}

	m_listView->SetModel(m_listViewModel.get());
	m_listView->SetDelegate(this);

	SelectFirstItem();
}

void SearchTabsDialog::SelectFirstItem()
{
	if (m_listViewModel->GetNumItems() > 0)
	{
		m_listView->SelectItem(m_listViewModel->GetItemAtIndex(0));
	}
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
	auto selectedItems = m_listView->GetSelectedItems();
	ListViewItem *currentItem = nullptr;

	if (selectedItems.size() == 1)
	{
		currentItem = selectedItems[0];
	}

	if (!currentItem)
	{
		currentItem = m_listView->MaybeGetFocusedItem();
	}

	if (!currentItem)
	{
		if (m_listViewModel->GetNumItems() > 0)
		{
			currentItem = m_listViewModel->GetItemAtIndex(0);
		}
	}

	if (!currentItem)
	{
		return;
	}

	int currentItemIndex = m_listViewModel->GetItemIndex(currentItem);
	int newIndex = currentItemIndex + (direction == MoveDirection::Up ? -1 : 1);

	if (newIndex < 0 || newIndex >= m_listViewModel->GetNumItems())
	{
		return;
	}

	auto *itemToSelect = m_listViewModel->GetItemAtIndex(newIndex);
	m_listView->SelectItem(itemToSelect);
	m_listView->EnsureItemVisible(itemToSelect);
}

void SearchTabsDialog::OnItemsActivated(const std::vector<ListViewItem *> &items)
{
	// Single selection is enabled for the listview. So, whenever a set of items are activated, that
	// set should only contain a single item.
	CHECK(items.size() == 1);
	SelectTabForItem(items[0]);
	DestroyWindow(m_hDlg);
}

void SearchTabsDialog::OnOk()
{
	auto selectedItems = m_listView->GetSelectedItems();

	if (selectedItems.size() == 1)
	{
		SelectTabForItem(selectedItems[0]);
	}

	DestroyWindow(m_hDlg);
}

void SearchTabsDialog::SelectTabForItem(ListViewItem *item)
{
	const auto *tab = m_listViewModel->GetTabForItem(item);
	tab->GetTabContainer()->SelectTab(*tab);
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

	m_persistentSettings->m_sortColumn = m_listViewModel->GetSortColumnId();
	m_persistentSettings->m_sortDirection = m_listViewModel->GetSortDirection();
	m_persistentSettings->m_searchTerm = m_model->GetSearchTerm();

	m_persistentSettings->m_bStateSaved = TRUE;
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
