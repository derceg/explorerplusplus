// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "OptionsDialog.h"
#include "AdvancedOptionsPage.h"
#include "App.h"
#include "AppearanceOptionsPage.h"
#include "CoreInterface.h"
#include "DarkModeManager.h"
#include "DefaultSettingsOptionsPage.h"
#include "FilesFoldersOptionsPage.h"
#include "FontsOptionsPage.h"
#include "GeneralOptionsPage.h"
#include "Icon.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "StartupOptionsPage.h"
#include "TabsOptionsPage.h"
#include "WindowOptionsPage.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/ScopedRedrawDisabler.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/WindowSubclass.h"
#include <boost/algorithm/string/predicate.hpp>

OptionsDialog::OptionsDialog(HINSTANCE resourceInstance, HWND parent, App *app, Config *config,
	CoreInterface *coreInterface) :
	ThemedDialog(app->GetResourceLoader(), resourceInstance, IDD_OPTIONS, parent,
		DialogSizingType::Both, app->GetThemeManager()),
	m_app(app),
	m_config(config),
	m_resourceInstance(resourceInstance),
	m_coreInterface(coreInterface)
{
}

INT_PTR OptionsDialog::OnInitDialog()
{
	SetupSearchField();

	if (!m_lastSelectedPageId)
	{
		m_lastSelectedPageId = m_pageMap.begin()->first;
	}

	SelectPage(*m_lastSelectedPageId);

	// Focus the treeview by default.
	SendMessage(m_hDlg, WM_NEXTDLGCTL,
		reinterpret_cast<WPARAM>(GetDlgItem(m_hDlg, IDC_OPTIONS_PAGES_TREE)), true);

	CenterWindow(GetParent(m_hDlg), m_hDlg);

	m_initializationFinished = true;

	return FALSE;
}

void OptionsDialog::SetupSearchField()
{
	auto searchField = GetDlgItem(m_hDlg, IDC_OPTIONS_SEARCH);
	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(searchField,
		std::bind_front(&OptionsDialog::SearchFieldWndProc, this)));

	auto placeholderText = m_resourceLoader->LoadString(IDS_OPTIONS_SEARCH_PLACEHOLDER_TEXT);
	SendMessage(searchField, EM_SETCUEBANNER, true,
		reinterpret_cast<LPARAM>(placeholderText.c_str()));
}

LRESULT OptionsDialog::SearchFieldWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_GETDLGCODE:
		switch (wParam)
		{
		case VK_RETURN:
			return DLGC_WANTALLKEYS;
		}
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_RETURN:
		{
			auto treeView = GetDlgItem(m_hDlg, IDC_OPTIONS_PAGES_TREE);
			SetFocus(treeView);
		}
			return 0;
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void OptionsDialog::AddDynamicControls()
{
	AddPages();
	AddPagesToTreeView();
}

wil::unique_hicon OptionsDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return m_resourceLoader->LoadIconFromPNGAndScale(Icon::Options, iconWidth, iconHeight);
}

std::vector<ResizableDialogControl> OptionsDialog::GetResizableControls()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_OPTIONS_PAGES_TREE), MovingType::None,
		SizingType::Vertical);
	controls.emplace_back(GetDlgItem(m_hDlg, IDOK), MovingType::Both, SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDCANCEL), MovingType::Both, SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDAPPLY), MovingType::Both, SizingType::None);

	for (const auto &page : m_pageMap | std::views::values)
	{
		controls.emplace_back(page->GetDialog(), MovingType::None, SizingType::Both);
	}

	return controls;
}

void OptionsDialog::AddPages()
{
	AddPage(std::make_unique<GeneralOptionsPage>(m_hDlg, m_resourceLoader, GetResourceInstance(),
		m_app, m_config, m_coreInterface, std::bind(&OptionsDialog::OnSettingChanged, this),
		m_tipWnd));
	AddPage(std::make_unique<StartupOptionsPage>(m_hDlg, m_resourceLoader, GetResourceInstance(),
		m_config, m_coreInterface, std::bind(&OptionsDialog::OnSettingChanged, this), m_tipWnd,
		m_app->GetDarkModeManager(), GetThemeManager()));
	AddPage(std::make_unique<AppearanceOptionsPage>(m_hDlg, m_resourceLoader, GetResourceInstance(),
		m_config, m_coreInterface, std::bind(&OptionsDialog::OnSettingChanged, this), m_tipWnd,
		m_app->GetDarkModeManager()));
	AddPage(std::make_unique<FontsOptionsPage>(m_hDlg, m_resourceLoader, GetResourceInstance(),
		m_config, m_coreInterface, std::bind(&OptionsDialog::OnSettingChanged, this), m_tipWnd));
	AddPage(std::make_unique<FilesFoldersOptionsPage>(m_hDlg, m_resourceLoader,
		GetResourceInstance(), m_config, m_coreInterface,
		std::bind(&OptionsDialog::OnSettingChanged, this), m_tipWnd));
	AddPage(std::make_unique<WindowOptionsPage>(m_hDlg, m_resourceLoader, GetResourceInstance(),
		m_config, m_coreInterface, std::bind(&OptionsDialog::OnSettingChanged, this), m_tipWnd));
	AddPage(std::make_unique<TabsOptionsPage>(m_hDlg, m_resourceLoader, GetResourceInstance(),
		m_config, m_coreInterface, std::bind(&OptionsDialog::OnSettingChanged, this), m_tipWnd));
	AddPage(std::make_unique<DefaultSettingsOptionsPage>(m_hDlg, m_resourceLoader,
		GetResourceInstance(), m_config, m_coreInterface,
		std::bind(&OptionsDialog::OnSettingChanged, this), m_tipWnd, GetThemeManager()));
	AddPage(std::make_unique<AdvancedOptionsPage>(m_hDlg, m_resourceLoader, GetResourceInstance(),
		m_config, m_coreInterface, std::bind(&OptionsDialog::OnSettingChanged, this), m_tipWnd));
}

void OptionsDialog::AddPage(std::unique_ptr<OptionsPage> page)
{
	page->InitializeDialog();

	auto treeView = GetDlgItem(m_hDlg, IDC_OPTIONS_PAGES_TREE);
	auto searchField = GetDlgItem(m_hDlg, IDC_OPTIONS_SEARCH);

	RECT searchFieldRect;
	GetWindowRect(searchField, &searchFieldRect);
	MapWindowPoints(HWND_DESKTOP, m_hDlg, reinterpret_cast<LPPOINT>(&searchFieldRect), 2);

	auto horizontalSpacing =
		DpiCompatibility::GetInstance().ScaleValue(m_hDlg, NAVIGATION_CONTENT_HORIZONTAL_SPACING);
	SetWindowPos(page->GetDialog(), treeView, searchFieldRect.right + horizontalSpacing,
		searchFieldRect.top, 0, 0, SWP_NOSIZE);

	int pageId = m_idCounter++;
	m_pageMap.insert({ pageId, std::move(page) });
}

void OptionsDialog::AddPagesToTreeView()
{
	auto treeView = GetDlgItem(m_hDlg, IDC_OPTIONS_PAGES_TREE);

	ScopedRedrawDisabler redrawDisabler(treeView);

	// As items are deleted, the selection will be automatically updated, but there's no need to
	// process the selection notification in that case.
	m_ignoreSelectionUpdate = true;
	TreeView_DeleteAllItems(treeView);
	m_ignoreSelectionUpdate = false;

	m_treeMap.clear();

	auto searchText = GetWindowString(GetDlgItem(m_hDlg, IDC_OPTIONS_SEARCH));
	auto filteredPages = m_pageMap
		| std::views::filter(
			std::bind(&OptionsDialog::PageFilter, std::placeholders::_1, searchText));

	for (const auto &item : filteredPages)
	{
		AddPageToTreeView(item.second.get(), item.first);
	}

	HWND noResultsControl = GetDlgItem(m_hDlg, IDC_OPTIONS_NO_RESULTS_FOUND);

	if (filteredPages.empty())
	{
		ShowWindow(noResultsControl, SW_SHOW);

		// The WS_EX_TRANSPARENT style set on the static control will ensure the
		// non-WS_EX_TRANSPARENT sibling windows are painted first, however, the static control will
		// still need to invalidated, if necessary.
		// When the static control switches from hidden to visible, it will be invalidated. However,
		// when the control is already visible, the ShowWindow() call above will have no effect, but
		// the treeview will have been invalidated by the calls above.
		// So, without this call, the static control would disappear if it was previously visible
		// and the new search term also had no matches (since only the treeview control would be
		// drawn).
		InvalidateRect(noResultsControl, nullptr, true);
	}
	else
	{
		ShowWindow(noResultsControl, SW_HIDE);
	}
}

bool OptionsDialog::PageFilter(const std::pair<const int, std::unique_ptr<OptionsPage>> &pageItem,
	const std::wstring &searchText)
{
	if (searchText.empty())
	{
		return true;
	}

	if (pageItem.second->DoesPageContainText(searchText,
			[](const std::wstring &input, const std::wstring &test)
			{ return boost::icontains(input, test); }))
	{
		return true;
	}

	return false;
}

HTREEITEM OptionsDialog::AddPageToTreeView(const OptionsPage *page, int pageId)
{
	auto title = page->GetTitle();

	TVITEMEX treeViewItem;
	treeViewItem.mask = TVIF_TEXT | TVIF_PARAM;
	treeViewItem.pszText = title.data();
	treeViewItem.lParam = pageId;

	TVINSERTSTRUCT insertInfo;
	insertInfo.hParent = nullptr;
	insertInfo.hInsertAfter = TVI_LAST;
	insertInfo.itemex = treeViewItem;

	auto treeView = GetDlgItem(m_hDlg, IDC_OPTIONS_PAGES_TREE);
	auto insertedItem = TreeView_InsertItem(treeView, &insertInfo);
	assert(insertedItem);

	if (m_currentPageId && *m_currentPageId == pageId)
	{
		TreeView_SelectItem(treeView, insertedItem);
	}

	m_treeMap.insert({ pageId, insertedItem });

	return insertedItem;
}

void OptionsDialog::SelectPage(int pageId)
{
	// This will trigger a TVN_SELCHANGED notification. The handler for that will take care of
	// updating the page visibility.
	auto treeView = GetDlgItem(m_hDlg, IDC_OPTIONS_PAGES_TREE);
	TreeView_SelectItem(treeView, m_treeMap.at(pageId));
}

INT_PTR OptionsDialog::OnCtlColorStatic(HWND hwnd, HDC hdc)
{
	UNREFERENCED_PARAMETER(hdc);

	// In dark mode, the background color in the static control will be the same as the background
	// color in the treeview, so there's no need to handle this message.
	if (m_app->GetDarkModeManager()->IsDarkModeEnabled())
	{
		// This will result in the default message handling being performed.
		return FALSE;
	}

	HWND noResultsControl = GetDlgItem(m_hDlg, IDC_OPTIONS_NO_RESULTS_FOUND);

	if (hwnd != noResultsControl)
	{
		return FALSE;
	}

	// The static control has the WS_EX_TRANSPARENT style set. That style will ensure that any
	// non-WS_EX_TRANSPARENT sibling windows are drawn first. In this case, it means that the
	// sibling window under the static control is guaranteed to be drawn first and there's no need
	// to paint a background. Returning HOLLOW_BRUSH ensures that no background will be painted.
	return reinterpret_cast<INT_PTR>(GetStockObject(HOLLOW_BRUSH));
}

INT_PTR OptionsDialog::OnNotify(NMHDR *nmhdr)
{
	switch (nmhdr->code)
	{
	case TVN_SELCHANGED:
		OnTreeViewSelectionChanged(reinterpret_cast<NMTREEVIEW *>(nmhdr));
		break;
	}

	return 0;
}

void OptionsDialog::OnTreeViewSelectionChanged(const NMTREEVIEW *changeInfo)
{
	if (m_ignoreSelectionUpdate)
	{
		return;
	}

	auto updatedPageId = static_cast<int>(changeInfo->itemNew.lParam);

	if (m_currentPageId && *m_currentPageId == updatedPageId)
	{
		return;
	}

	if (m_currentPageId)
	{
		ShowWindow(m_pageMap.at(*m_currentPageId)->GetDialog(), SW_HIDE);
	}

	ShowWindow(m_pageMap.at(updatedPageId)->GetDialog(), SW_SHOW);
	m_currentPageId = updatedPageId;
}

INT_PTR OptionsDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (HIWORD(wParam) != 0)
	{
		switch (HIWORD(wParam))
		{
		case EN_CHANGE:
		{
			if (LOWORD(wParam) == IDC_OPTIONS_SEARCH)
			{
				OnSearchTextChanged();
			}
		}
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

		case IDAPPLY:
			OnApply();
			break;

		case IDCANCEL:
			OnCancel();
			break;
		}
	}

	return 0;
}

void OptionsDialog::OnSearchTextChanged()
{
	AddPagesToTreeView();
}

void OptionsDialog::OnOk()
{
	OnApply();
	DestroyWindow(m_hDlg);
}

void OptionsDialog::OnApply()
{
	for (auto &page : m_pageMap | std::views::values)
	{
		page->SaveSettings();
	}

	EnableWindow(GetDlgItem(m_hDlg, IDAPPLY), false);
}

void OptionsDialog::OnCancel()
{
	DestroyWindow(m_hDlg);
}

INT_PTR OptionsDialog::OnClose()
{
	DestroyWindow(m_hDlg);
	return 0;
}

void OptionsDialog::OnSettingChanged()
{
	if (m_initializationFinished)
	{
		EnableWindow(GetDlgItem(m_hDlg, IDAPPLY), true);
	}
}

INT_PTR OptionsDialog::OnDestroy()
{
	m_lastSelectedPageId = *m_currentPageId;
	return 0;
}

INT_PTR OptionsDialog::OnNcDestroy()
{
	delete this;

	return 0;
}
