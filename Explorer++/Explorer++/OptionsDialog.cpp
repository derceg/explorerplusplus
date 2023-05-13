// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "OptionsDialog.h"
#include "AdvancedOptionsPage.h"
#include "AppearanceOptionsPage.h"
#include "CoreInterface.h"
#include "DefaultSettingsOptionsPage.h"
#include "FilesFoldersOptionsPage.h"
#include "GeneralOptionsPage.h"
#include "Icon.h"
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "TabsOptionsPage.h"
#include "WindowOptionsPage.h"
#include "../Helper/WindowHelper.h"

OptionsDialog::OptionsDialog(HINSTANCE resourceInstance, HWND parent,
	std::shared_ptr<Config> config, CoreInterface *coreInterface) :
	DarkModeDialogBase(resourceInstance, IDD_OPTIONS, parent, DialogSizingType::Both),
	m_config(config),
	m_resourceInstance(resourceInstance),
	m_coreInterface(coreInterface)
{
}

INT_PTR OptionsDialog::OnInitDialog()
{
	SetWindowTheme(GetDlgItem(m_hDlg, IDC_SETTINGS_PAGES_TREE), L"Explorer", nullptr);

	AllowDarkModeForTreeView(IDC_SETTINGS_PAGES_TREE);
	AllowDarkModeForControls({ IDAPPLY });

	if (!m_lastSelectedPageId)
	{
		m_lastSelectedPageId = m_pageMap.begin()->first;
	}

	SelectPage(*m_lastSelectedPageId);

	// Focus the treeview by default.
	SendMessage(m_hDlg, WM_NEXTDLGCTL,
		reinterpret_cast<WPARAM>(GetDlgItem(m_hDlg, IDC_SETTINGS_PAGES_TREE)), true);

	CenterWindow(GetParent(m_hDlg), m_hDlg);

	m_initializationFinished = true;

	return FALSE;
}

void OptionsDialog::AddDynamicControls()
{
	AddPages();
}

wil::unique_hicon OptionsDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return m_coreInterface->GetIconResourceLoader()->LoadIconFromPNGAndScale(Icon::Options,
		iconWidth, iconHeight);
}

std::vector<ResizableDialogControl> OptionsDialog::GetResizableControls()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_SETTINGS_PAGES_TREE), MovingType::None,
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
	AddPage(std::make_unique<GeneralOptionsPage>(m_hDlg, GetResourceInstance(), m_config.get(),
		m_coreInterface, std::bind(&OptionsDialog::OnSettingChanged, this), m_tipWnd));
	AddPage(std::make_unique<AppearanceOptionsPage>(m_hDlg, GetResourceInstance(), m_config.get(),
		m_coreInterface, std::bind(&OptionsDialog::OnSettingChanged, this), m_tipWnd));
	AddPage(std::make_unique<FilesFoldersOptionsPage>(m_hDlg, GetResourceInstance(), m_config.get(),
		m_coreInterface, std::bind(&OptionsDialog::OnSettingChanged, this), m_tipWnd));
	AddPage(std::make_unique<WindowOptionsPage>(m_hDlg, GetResourceInstance(), m_config.get(),
		m_coreInterface, std::bind(&OptionsDialog::OnSettingChanged, this), m_tipWnd));
	AddPage(std::make_unique<TabsOptionsPage>(m_hDlg, GetResourceInstance(), m_config.get(),
		m_coreInterface, std::bind(&OptionsDialog::OnSettingChanged, this), m_tipWnd));
	AddPage(
		std::make_unique<DefaultSettingsOptionsPage>(m_hDlg, GetResourceInstance(), m_config.get(),
			m_coreInterface, std::bind(&OptionsDialog::OnSettingChanged, this), m_tipWnd));
	AddPage(std::make_unique<AdvancedOptionsPage>(m_hDlg, GetResourceInstance(), m_config.get(),
		m_coreInterface, std::bind(&OptionsDialog::OnSettingChanged, this), m_tipWnd));
}

void OptionsDialog::AddPage(std::unique_ptr<OptionsPage> page)
{
	auto treeView = GetDlgItem(m_hDlg, IDC_SETTINGS_PAGES_TREE);

	RECT treeViewRect;
	GetWindowRect(treeView, &treeViewRect);
	MapWindowPoints(HWND_DESKTOP, m_hDlg, reinterpret_cast<LPPOINT>(&treeViewRect), 2);

	page->InitializeDialog();

	SetWindowPos(page->GetDialog(), nullptr, treeViewRect.right + TREEVIEW_PAGE_HORIZONTAL_SPACING,
		treeViewRect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	auto title = page->GetTitle();
	int pageId = m_idCounter++;

	TVITEMEX treeViewItem;
	treeViewItem.mask = TVIF_TEXT | TVIF_PARAM;
	treeViewItem.pszText = title.data();
	treeViewItem.lParam = pageId;

	TVINSERTSTRUCT insertInfo;
	insertInfo.hParent = nullptr;
	insertInfo.hInsertAfter = TVI_LAST;
	insertInfo.itemex = treeViewItem;

	auto insertedItem = TreeView_InsertItem(treeView, &insertInfo);

	m_treeMap.insert({ pageId, insertedItem });
	m_pageMap.insert({ pageId, std::move(page) });
}

void OptionsDialog::SelectPage(int id)
{
	// This will trigger a TVN_SELCHANGED notification. The handler for that will take care of
	// updating the page visibility.
	auto treeView = GetDlgItem(m_hDlg, IDC_SETTINGS_PAGES_TREE);
	TreeView_SelectItem(treeView, m_treeMap.at(id));
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
	if (m_currentPageId)
	{
		ShowWindow(m_pageMap.at(*m_currentPageId)->GetDialog(), SW_HIDE);
	}

	int updatedPageId = static_cast<int>(changeInfo->itemNew.lParam);
	ShowWindow(m_pageMap.at(updatedPageId)->GetDialog(), SW_SHOW);
	m_currentPageId = updatedPageId;
}

INT_PTR OptionsDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (HIWORD(wParam) == 0 || HIWORD(wParam) == 1)
	{
		return HandleMenuOrAccelerator(wParam);
	}

	return 1;
}

LRESULT OptionsDialog::HandleMenuOrAccelerator(WPARAM wParam)
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

	return 0;
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

	m_coreInterface->SaveAllSettings();

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
