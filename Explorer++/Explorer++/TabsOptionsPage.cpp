// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabsOptionsPage.h"
#include "Config.h"
#include "MainResource.h"
#include "../Helper/ResizableDialogHelper.h"

TabsOptionsPage::TabsOptionsPage(HWND parent, const ResourceLoader *resourceLoader, Config *config,
	CoreInterface *coreInterface, SettingChangedCallback settingChangedCallback,
	HWND tooltipWindow) :
	OptionsPage(IDD_OPTIONS_TABS, IDS_OPTIONS_TABS_TITLE, parent, resourceLoader, config,
		coreInterface, settingChangedCallback, tooltipWindow)
{
}

std::unique_ptr<ResizableDialogHelper> TabsOptionsPage::InitializeResizeDialogHelper()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_TABS_TASKBARTHUMBNAILS), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_TABS_CLOSECONFIRMATION), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_TABS_OPENNEXTTOCURRENT), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_SETTINGS_CHECK_ALWAYSNEWTAB),
		MovingType::None, SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_TABS_DOUBLECLICKCLOSE), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_TABS_CLOSEMAINWINDOW), MovingType::None,
		SizingType::Horizontal);
	return std::make_unique<ResizableDialogHelper>(GetDialog(), controls);
}

void TabsOptionsPage::InitializeControls()
{
	if (m_config->showTaskbarThumbnails)
	{
		CheckDlgButton(GetDialog(), IDC_TABS_TASKBARTHUMBNAILS, BST_CHECKED);
	}

	if (m_config->confirmCloseTabs)
	{
		CheckDlgButton(GetDialog(), IDC_TABS_CLOSECONFIRMATION, BST_CHECKED);
	}

	if (m_config->openNewTabNextToCurrent)
	{
		CheckDlgButton(GetDialog(), IDC_TABS_OPENNEXTTOCURRENT, BST_CHECKED);
	}

	if (m_config->alwaysOpenNewTab)
	{
		CheckDlgButton(GetDialog(), IDC_SETTINGS_CHECK_ALWAYSNEWTAB, BST_CHECKED);
	}

	if (m_config->doubleClickTabClose)
	{
		CheckDlgButton(GetDialog(), IDC_TABS_DOUBLECLICKCLOSE, BST_CHECKED);
	}

	if (m_config->closeMainWindowOnTabClose)
	{
		CheckDlgButton(GetDialog(), IDC_TABS_CLOSEMAINWINDOW, BST_CHECKED);
	}
}

void TabsOptionsPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam))
	{
	case IDC_TABS_TASKBARTHUMBNAILS:
	case IDC_TABS_CLOSECONFIRMATION:
	case IDC_TABS_OPENNEXTTOCURRENT:
	case IDC_SETTINGS_CHECK_ALWAYSNEWTAB:
	case IDC_TABS_DOUBLECLICKCLOSE:
	case IDC_TABS_CLOSEMAINWINDOW:
		m_settingChangedCallback();
		break;
	}
}

void TabsOptionsPage::SaveSettings()
{
	m_config->showTaskbarThumbnails =
		(IsDlgButtonChecked(GetDialog(), IDC_TABS_TASKBARTHUMBNAILS) == BST_CHECKED);

	m_config->confirmCloseTabs =
		(IsDlgButtonChecked(GetDialog(), IDC_TABS_CLOSECONFIRMATION) == BST_CHECKED);

	m_config->openNewTabNextToCurrent =
		(IsDlgButtonChecked(GetDialog(), IDC_TABS_OPENNEXTTOCURRENT) == BST_CHECKED);

	m_config->alwaysOpenNewTab =
		(IsDlgButtonChecked(GetDialog(), IDC_SETTINGS_CHECK_ALWAYSNEWTAB) == BST_CHECKED);

	m_config->doubleClickTabClose =
		(IsDlgButtonChecked(GetDialog(), IDC_TABS_DOUBLECLICKCLOSE) == BST_CHECKED);

	m_config->closeMainWindowOnTabClose =
		(IsDlgButtonChecked(GetDialog(), IDC_TABS_CLOSEMAINWINDOW) == BST_CHECKED);
}
