// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DefaultSettingsOptionsPage.h"
#include "Config.h"
#include "MainResource.h"
#include "SetDefaultColumnsDialog.h"
#include "SortModeMenuMappings.h"
#include "ViewModeHelper.h"
#include "../Helper/Controls.h"
#include "../Helper/ResizableDialogHelper.h"

namespace
{

// Sort modes to present in the default settings comboboxes. These are the most commonly used sort
// modes for regular folders.
// clang-format off
const std::array<SortMode, 13> DEFAULT_SORT_MODES = {
	SortMode::Name,
	SortMode::DateModified,
	SortMode::Size,
	SortMode::Type,
	SortMode::Attributes,
	SortMode::RealSize,
	SortMode::ShortName,
	SortMode::Owner,
	SortMode::Extension,
	SortMode::Created,
	SortMode::Accessed,
	SortMode::HardLinks,
	SortMode::Comments
};
// clang-format on

UINT GetSortModeStringIndex(SortMode sortMode)
{
	UINT menuItemId = GetMenuItemIdForSortMode(sortMode);

	switch (menuItemId)
	{
	case IDM_SORTBY_NAME:
		return IDS_SORTBY_NAME;
	case IDM_SORTBY_SIZE:
		return IDS_SORTBY_SIZE;
	case IDM_SORTBY_TYPE:
		return IDS_SORTBY_TYPE;
	case IDM_SORTBY_DATEMODIFIED:
		return IDS_SORTBY_DATEMODIFIED;
	case IDM_SORTBY_TOTALSIZE:
		return IDS_SORTBY_TOTALSIZE;
	case IDM_SORTBY_FREESPACE:
		return IDS_SORTBY_FREESPACE;
	case IDM_SORTBY_DATEDELETED:
		return IDS_SORTBY_DATEDELETED;
	case IDM_SORTBY_ORIGINALLOCATION:
		return IDS_SORTBY_ORIGINALLOCATION;
	case IDM_SORTBY_ATTRIBUTES:
		return IDS_SORTBY_ATTRIBUTES;
	case IDM_SORTBY_REALSIZE:
		return IDS_SORTBY_REALSIZE;
	case IDM_SORTBY_SHORTNAME:
		return IDS_SORTBY_SHORTNAME;
	case IDM_SORTBY_OWNER:
		return IDS_SORTBY_OWNER;
	case IDM_SORTBY_PRODUCTNAME:
		return IDS_SORTBY_PRODUCTNAME;
	case IDM_SORTBY_COMPANY:
		return IDS_SORTBY_COMPANY;
	case IDM_SORTBY_DESCRIPTION:
		return IDS_SORTBY_DESCRIPTION;
	case IDM_SORTBY_FILEVERSION:
		return IDS_SORTBY_FILEVERSION;
	case IDM_SORTBY_PRODUCTVERSION:
		return IDS_SORTBY_PRODUCTVERSION;
	case IDM_SORTBY_SHORTCUTTO:
		return IDS_SORTBY_SHORTCUTTO;
	case IDM_SORTBY_HARDLINKS:
		return IDS_SORTBY_HARDLINKS;
	case IDM_SORTBY_EXTENSION:
		return IDS_SORTBY_EXTENSION;
	case IDM_SORTBY_CREATED:
		return IDS_SORTBY_CREATED;
	case IDM_SORTBY_ACCESSED:
		return IDS_SORTBY_ACCESSED;
	case IDM_SORTBY_TITLE:
		return IDS_SORTBY_TITLE;
	case IDM_SORTBY_SUBJECT:
		return IDS_SORTBY_SUBJECT;
	case IDM_SORTBY_AUTHORS:
		return IDS_SORTBY_AUTHORS;
	case IDM_SORTBY_KEYWORDS:
		return IDS_SORTBY_KEYWORDS;
	case IDM_SORTBY_COMMENTS:
		return IDS_SORTBY_COMMENT;
	case IDM_SORTBY_CAMERAMODEL:
		return IDS_SORTBY_CAMERAMODEL;
	case IDM_SORTBY_DATETAKEN:
		return IDS_SORTBY_DATETAKEN;
	case IDM_SORTBY_WIDTH:
		return IDS_SORTBY_WIDTH;
	case IDM_SORTBY_HEIGHT:
		return IDS_SORTBY_HEIGHT;
	default:
		assert(false);
		return IDS_SORTBY_NAME;
	}
}

}

DefaultSettingsOptionsPage::DefaultSettingsOptionsPage(HWND parent,
	const ResourceLoader *resourceLoader, Config *config,
	SettingChangedCallback settingChangedCallback, HWND tooltipWindow) :
	OptionsPage(IDD_OPTIONS_DEFAULT, IDS_OPTIONS_DEFAULT_TITLE, parent, resourceLoader, config,
		settingChangedCallback, tooltipWindow)
{
}

std::unique_ptr<ResizableDialogHelper> DefaultSettingsOptionsPage::InitializeResizeDialogHelper()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_STATIC_DEFAULT_SETTINGS_NOTICE),
		MovingType::None, SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_SHOWHIDDENGLOBAL), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_AUTOARRANGEGLOBAL), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_SHOWINGROUPSGLOBAL), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_SORTASCENDINGGLOBAL), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_GROUP_SORT_ASCENDING_GLOBAL),
		MovingType::None, SizingType::Horizontal);
	return std::make_unique<ResizableDialogHelper>(GetDialog(), controls);
}

void DefaultSettingsOptionsPage::InitializeControls()
{
	if (m_config->defaultFolderSettings.showHidden)
	{
		CheckDlgButton(GetDialog(), IDC_SHOWHIDDENGLOBAL, BST_CHECKED);
	}

	if (m_config->defaultFolderSettings.autoArrangeEnabled)
	{
		CheckDlgButton(GetDialog(), IDC_AUTOARRANGEGLOBAL, BST_CHECKED);
	}

	if (m_config->defaultFolderSettings.sortDirection == +SortDirection::Ascending)
	{
		CheckDlgButton(GetDialog(), IDC_SORTASCENDINGGLOBAL, BST_CHECKED);
	}

	if (m_config->defaultFolderSettings.showInGroups)
	{
		CheckDlgButton(GetDialog(), IDC_SHOWINGROUPSGLOBAL, BST_CHECKED);
	}

	if (m_config->defaultFolderSettings.groupSortDirection == +SortDirection::Ascending)
	{
		CheckDlgButton(GetDialog(), IDC_GROUP_SORT_ASCENDING_GLOBAL, BST_CHECKED);
	}

	std::vector<ComboBoxItem> viewModeItems;

	for (auto viewMode : VIEW_MODES)
	{
		viewModeItems.emplace_back(viewMode, GetViewModeMenuText(m_resourceLoader, viewMode));
	}

	AddItemsToComboBox(GetDlgItem(GetDialog(), IDC_OPTIONS_DEFAULT_VIEW), viewModeItems,
		m_config->defaultFolderSettings.viewMode);

	std::vector<ComboBoxItem> sortModeItems;

	for (auto sortMode : DEFAULT_SORT_MODES)
	{
		sortModeItems.emplace_back(sortMode,
			m_resourceLoader->LoadString(GetSortModeStringIndex(sortMode)));
	}

	AddItemsToComboBox(GetDlgItem(GetDialog(), IDC_OPTIONS_DEFAULT_SORT_MODE), sortModeItems,
		m_config->defaultFolderSettings.sortMode);
	AddItemsToComboBox(GetDlgItem(GetDialog(), IDC_OPTIONS_DEFAULT_GROUP_MODE), sortModeItems,
		m_config->defaultFolderSettings.groupMode);
}

void DefaultSettingsOptionsPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (HIWORD(wParam) != 0)
	{
		switch (HIWORD(wParam))
		{
		case CBN_SELCHANGE:
			m_settingChangedCallback();
			break;
		}
	}
	else
	{
		switch (LOWORD(wParam))
		{
		case IDC_SHOWHIDDENGLOBAL:
		case IDC_AUTOARRANGEGLOBAL:
		case IDC_SORTASCENDINGGLOBAL:
		case IDC_SHOWINGROUPSGLOBAL:
		case IDC_GROUP_SORT_ASCENDING_GLOBAL:
			m_settingChangedCallback();
			break;

		case IDC_BUTTON_DEFAULTCOLUMNS:
		{
			auto *setDefaultColumnsDialog = SetDefaultColumnsDialog::Create(m_resourceLoader,
				GetDialog(), m_config->globalFolderSettings.folderColumns);
			setDefaultColumnsDialog->ShowModalDialog();
		}
		break;
		}
	}
}

void DefaultSettingsOptionsPage::SaveSettings()
{
	m_config->defaultFolderSettings.showHidden =
		(IsDlgButtonChecked(GetDialog(), IDC_SHOWHIDDENGLOBAL) == BST_CHECKED);

	m_config->defaultFolderSettings.autoArrangeEnabled =
		(IsDlgButtonChecked(GetDialog(), IDC_AUTOARRANGEGLOBAL) == BST_CHECKED);

	bool sortAscending = (IsDlgButtonChecked(GetDialog(), IDC_SORTASCENDINGGLOBAL) == BST_CHECKED);
	m_config->defaultFolderSettings.sortDirection =
		sortAscending ? SortDirection::Ascending : SortDirection::Descending;

	m_config->defaultFolderSettings.showInGroups =
		(IsDlgButtonChecked(GetDialog(), IDC_SHOWINGROUPSGLOBAL) == BST_CHECKED);

	bool groupSortAscending =
		(IsDlgButtonChecked(GetDialog(), IDC_GROUP_SORT_ASCENDING_GLOBAL) == BST_CHECKED);
	m_config->defaultFolderSettings.groupSortDirection =
		groupSortAscending ? SortDirection::Ascending : SortDirection::Descending;

	HWND hComboBox = GetDlgItem(GetDialog(), IDC_OPTIONS_DEFAULT_VIEW);
	int selectedIndex = static_cast<int>(SendMessage(hComboBox, CB_GETCURSEL, 0, 0));
	m_config->defaultFolderSettings.viewMode = ViewMode::_from_integral(
		static_cast<int>(SendMessage(hComboBox, CB_GETITEMDATA, selectedIndex, 0)));

	HWND hSortModeComboBox = GetDlgItem(GetDialog(), IDC_OPTIONS_DEFAULT_SORT_MODE);
	int sortModeIndex = static_cast<int>(SendMessage(hSortModeComboBox, CB_GETCURSEL, 0, 0));
	m_config->defaultFolderSettings.sortMode = SortMode::_from_integral(
		static_cast<int>(SendMessage(hSortModeComboBox, CB_GETITEMDATA, sortModeIndex, 0)));

	HWND hGroupModeComboBox = GetDlgItem(GetDialog(), IDC_OPTIONS_DEFAULT_GROUP_MODE);
	int groupModeIndex = static_cast<int>(SendMessage(hGroupModeComboBox, CB_GETCURSEL, 0, 0));
	m_config->defaultFolderSettings.groupMode = SortMode::_from_integral(
		static_cast<int>(SendMessage(hGroupModeComboBox, CB_GETITEMDATA, groupModeIndex, 0)));
}
