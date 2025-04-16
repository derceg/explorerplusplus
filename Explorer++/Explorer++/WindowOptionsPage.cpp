// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "WindowOptionsPage.h"
#include "Config.h"
#include "MainResource.h"
#include "../Helper/ResizableDialogHelper.h"

WindowOptionsPage::WindowOptionsPage(HWND parent, const ResourceLoader *resourceLoader,
	Config *config, SettingChangedCallback settingChangedCallback, HWND tooltipWindow) :
	OptionsPage(IDD_OPTIONS_WINDOW, IDS_OPTIONS_WINDOW_TITLE, parent, resourceLoader, config,
		settingChangedCallback, tooltipWindow)
{
}

std::unique_ptr<ResizableDialogHelper> WindowOptionsPage::InitializeResizeDialogHelper()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_GROUP_GENERAL), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTION_MULTIPLEINSTANCES), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTION_LARGETOOLBARICONS), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTION_ALWAYSSHOWTABBAR), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTION_SHOWTABBARATBOTTOM), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTION_EXTENDTABCONTROL), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_SETTINGS_CHECK_TITLEPATH), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTION_USERNAMEINTITLEBAR), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTION_PRIVILEGELEVELINTITLEBAR),
		MovingType::None, SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_GROUP_MAIN_PANE), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTION_GRIDLINES), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTION_CHECKBOXSELECTION), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTION_FULLROWSELECT), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_GROUP_NAVIGATION_PANE), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTION_SYNCTREEVIEW), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTION_TREEVIEWSELECTIONEXPAND),
		MovingType::None, SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTION_TREEVIEWDELAY), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_GROUP_DISPLAY_WINDOW), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTION_FILEPREVIEWS), MovingType::None,
		SizingType::Horizontal);
	return std::make_unique<ResizableDialogHelper>(GetDialog(), controls);
}

void WindowOptionsPage::InitializeControls()
{
	if (m_config->allowMultipleInstances)
	{
		CheckDlgButton(GetDialog(), IDC_OPTION_MULTIPLEINSTANCES, BST_CHECKED);
	}

	if (m_config->useLargeToolbarIcons.get())
	{
		CheckDlgButton(GetDialog(), IDC_OPTION_LARGETOOLBARICONS, BST_CHECKED);
	}

	if (m_config->alwaysShowTabBar.get())
	{
		CheckDlgButton(GetDialog(), IDC_OPTION_ALWAYSSHOWTABBAR, BST_CHECKED);
	}

	if (m_config->showTabBarAtBottom.get())
	{
		CheckDlgButton(GetDialog(), IDC_OPTION_SHOWTABBARATBOTTOM, BST_CHECKED);
	}

	if (m_config->extendTabControl.get())
	{
		CheckDlgButton(GetDialog(), IDC_OPTION_EXTENDTABCONTROL, BST_CHECKED);
	}

	if (m_config->showFilePreviews)
	{
		CheckDlgButton(GetDialog(), IDC_OPTION_FILEPREVIEWS, BST_CHECKED);
	}

	if (m_config->showFullTitlePath.get())
	{
		CheckDlgButton(GetDialog(), IDC_SETTINGS_CHECK_TITLEPATH, BST_CHECKED);
	}

	if (m_config->showUserNameInTitleBar.get())
	{
		CheckDlgButton(GetDialog(), IDC_OPTION_USERNAMEINTITLEBAR, BST_CHECKED);
	}

	if (m_config->showPrivilegeLevelInTitleBar.get())
	{
		CheckDlgButton(GetDialog(), IDC_OPTION_PRIVILEGELEVELINTITLEBAR, BST_CHECKED);
	}

	if (m_config->synchronizeTreeview.get())
	{
		CheckDlgButton(GetDialog(), IDC_OPTION_SYNCTREEVIEW, BST_CHECKED);
	}

	if (m_config->treeViewAutoExpandSelected)
	{
		CheckDlgButton(GetDialog(), IDC_OPTION_TREEVIEWSELECTIONEXPAND, BST_CHECKED);
	}

	if (!m_config->treeViewDelayEnabled)
	{
		CheckDlgButton(GetDialog(), IDC_OPTION_TREEVIEWDELAY, BST_CHECKED);
	}

	if (m_config->globalFolderSettings.showGridlines.get())
	{
		CheckDlgButton(GetDialog(), IDC_OPTION_GRIDLINES, BST_CHECKED);
	}

	if (m_config->checkBoxSelection.get())
	{
		CheckDlgButton(GetDialog(), IDC_OPTION_CHECKBOXSELECTION, BST_CHECKED);
	}

	if (m_config->useFullRowSelect.get())
	{
		CheckDlgButton(GetDialog(), IDC_OPTION_FULLROWSELECT, BST_CHECKED);
	}
}

void WindowOptionsPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam))
	{
	case IDC_OPTION_MULTIPLEINSTANCES:
	case IDC_OPTION_LARGETOOLBARICONS:
	case IDC_OPTION_ALWAYSSHOWTABBAR:
	case IDC_OPTION_SHOWTABBARATBOTTOM:
	case IDC_OPTION_FILEPREVIEWS:
	case IDC_SETTINGS_CHECK_TITLEPATH:
	case IDC_OPTION_USERNAMEINTITLEBAR:
	case IDC_OPTION_PRIVILEGELEVELINTITLEBAR:
	case IDC_OPTION_SYNCTREEVIEW:
	case IDC_OPTION_TREEVIEWSELECTIONEXPAND:
	case IDC_OPTION_TREEVIEWDELAY:
	case IDC_OPTION_EXTENDTABCONTROL:
	case IDC_OPTION_GRIDLINES:
	case IDC_OPTION_CHECKBOXSELECTION:
	case IDC_OPTION_FULLROWSELECT:
		m_settingChangedCallback();
		break;
	}
}

void WindowOptionsPage::SaveSettings()
{
	m_config->allowMultipleInstances =
		(IsDlgButtonChecked(GetDialog(), IDC_OPTION_MULTIPLEINSTANCES) == BST_CHECKED);

	m_config->alwaysShowTabBar.set(
		IsDlgButtonChecked(GetDialog(), IDC_OPTION_ALWAYSSHOWTABBAR) == BST_CHECKED);

	m_config->showTabBarAtBottom.set(
		(IsDlgButtonChecked(GetDialog(), IDC_OPTION_SHOWTABBARATBOTTOM) == BST_CHECKED));

	m_config->extendTabControl.set(
		(IsDlgButtonChecked(GetDialog(), IDC_OPTION_EXTENDTABCONTROL) == BST_CHECKED));

	m_config->showFilePreviews =
		(IsDlgButtonChecked(GetDialog(), IDC_OPTION_FILEPREVIEWS) == BST_CHECKED);

	m_config->showFullTitlePath.set(
		IsDlgButtonChecked(GetDialog(), IDC_SETTINGS_CHECK_TITLEPATH) == BST_CHECKED);

	m_config->showUserNameInTitleBar.set(
		IsDlgButtonChecked(GetDialog(), IDC_OPTION_USERNAMEINTITLEBAR) == BST_CHECKED);

	m_config->showPrivilegeLevelInTitleBar.set(
		IsDlgButtonChecked(GetDialog(), IDC_OPTION_PRIVILEGELEVELINTITLEBAR) == BST_CHECKED);

	m_config->synchronizeTreeview =
		(IsDlgButtonChecked(GetDialog(), IDC_OPTION_SYNCTREEVIEW) == BST_CHECKED);

	m_config->treeViewAutoExpandSelected =
		(IsDlgButtonChecked(GetDialog(), IDC_OPTION_TREEVIEWSELECTIONEXPAND) == BST_CHECKED);

	m_config->treeViewDelayEnabled =
		!(IsDlgButtonChecked(GetDialog(), IDC_OPTION_TREEVIEWDELAY) == BST_CHECKED);

	m_config->globalFolderSettings.showGridlines =
		(IsDlgButtonChecked(GetDialog(), IDC_OPTION_GRIDLINES) == BST_CHECKED);

	m_config->checkBoxSelection =
		(IsDlgButtonChecked(GetDialog(), IDC_OPTION_CHECKBOXSELECTION) == BST_CHECKED);

	m_config->useFullRowSelect =
		(IsDlgButtonChecked(GetDialog(), IDC_OPTION_FULLROWSELECT) == BST_CHECKED);

	m_config->useLargeToolbarIcons.set(
		IsDlgButtonChecked(GetDialog(), IDC_OPTION_LARGETOOLBARICONS) == BST_CHECKED);
}
