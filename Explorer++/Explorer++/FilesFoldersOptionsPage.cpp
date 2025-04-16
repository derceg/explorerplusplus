// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FilesFoldersOptionsPage.h"
#include "Config.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "TabContainerImpl.h"
#include "../Helper/Controls.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/ResizableDialogHelper.h"
#include <boost/range/adaptor/map.hpp>
#include <glog/logging.h>

FilesFoldersOptionsPage::FilesFoldersOptionsPage(HWND parent, const ResourceLoader *resourceLoader,
	Config *config, CoreInterface *coreInterface, SettingChangedCallback settingChangedCallback,
	HWND tooltipWindow) :
	OptionsPage(IDD_OPTIONS_FILES_FOLDERS, IDS_OPTIONS_FILES_FOLDERS_TITLE, parent, resourceLoader,
		config, coreInterface, settingChangedCallback, tooltipWindow)
{
}

std::unique_ptr<ResizableDialogHelper> FilesFoldersOptionsPage::InitializeResizeDialogHelper()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_SETTINGS_CHECK_SYSTEMFILES), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_SETTINGS_CHECK_EXTENSIONS), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_SETTINGS_CHECK_LINK), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_SETTINGS_CHECK_INSERTSORTED),
		MovingType::None, SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_SETTINGS_CHECK_SINGLECLICK), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_SETTINGS_CHECK_EXISTINGFILESCONFIRMATION),
		MovingType::None, SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_SETTINGS_CHECK_FOLDERSIZES), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_SETTINGS_CHECK_FOLDERSIZESNETWORKREMOVABLE),
		MovingType::None, SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_SETTINGS_CHECK_CONTAINER_FILES),
		MovingType::None, SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_SETTINGS_CHECK_FRIENDLYDATES),
		MovingType::None, SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTIONS_CHECK_SHOWINFOTIPS), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTIONS_RADIO_SYSTEMINFOTIPS),
		MovingType::None, SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTIONS_RADIO_CUSTOMINFOTIPS),
		MovingType::None, SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_DISPLAY_MIXED_FILES_AND_FOLDERS),
		MovingType::None, SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_USE_NATURAL_SORT_ORDER), MovingType::None,
		SizingType::Horizontal);
	return std::make_unique<ResizableDialogHelper>(GetDialog(), controls);
}

void FilesFoldersOptionsPage::InitializeControls()
{
	if (m_config->globalFolderSettings.hideSystemFiles)
	{
		CheckDlgButton(GetDialog(), IDC_SETTINGS_CHECK_SYSTEMFILES, BST_CHECKED);
	}

	if (!m_config->globalFolderSettings.showExtensions)
	{
		CheckDlgButton(GetDialog(), IDC_SETTINGS_CHECK_EXTENSIONS, BST_CHECKED);
	}

	if (m_config->globalFolderSettings.hideLinkExtension)
	{
		CheckDlgButton(GetDialog(), IDC_SETTINGS_CHECK_LINK, BST_CHECKED);
	}

	if (m_config->globalFolderSettings.insertSorted)
	{
		CheckDlgButton(GetDialog(), IDC_SETTINGS_CHECK_INSERTSORTED, BST_CHECKED);
	}

	if (m_config->globalFolderSettings.oneClickActivate.get())
	{
		CheckDlgButton(GetDialog(), IDC_SETTINGS_CHECK_SINGLECLICK, BST_CHECKED);
	}

	SetDlgItemInt(GetDialog(), IDC_OPTIONS_HOVER_TIME,
		m_config->globalFolderSettings.oneClickActivateHoverTime.get(), FALSE);
	EnableWindow(GetDlgItem(GetDialog(), IDC_OPTIONS_HOVER_TIME),
		m_config->globalFolderSettings.oneClickActivate.get());
	EnableWindow(GetDlgItem(GetDialog(), IDC_LABEL_HOVER_TIME),
		m_config->globalFolderSettings.oneClickActivate.get());

	if (m_config->overwriteExistingFilesConfirmation)
	{
		CheckDlgButton(GetDialog(), IDC_SETTINGS_CHECK_EXISTINGFILESCONFIRMATION, BST_CHECKED);
	}

	if (m_config->globalFolderSettings.showFolderSizes)
	{
		CheckDlgButton(GetDialog(), IDC_SETTINGS_CHECK_FOLDERSIZES, BST_CHECKED);
	}

	if (m_config->globalFolderSettings.disableFolderSizesNetworkRemovable)
	{
		CheckDlgButton(GetDialog(), IDC_SETTINGS_CHECK_FOLDERSIZESNETWORKREMOVABLE, BST_CHECKED);
	}

	if (m_config->globalFolderSettings.forceSize)
	{
		CheckDlgButton(GetDialog(), IDC_SETTINGS_CHECK_FORCESIZE, BST_CHECKED);
	}

	if (m_config->openContainerFiles)
	{
		CheckDlgButton(GetDialog(), IDC_SETTINGS_CHECK_CONTAINER_FILES, BST_CHECKED);
	}

	if (m_config->globalFolderSettings.showFriendlyDates)
	{
		CheckDlgButton(GetDialog(), IDC_SETTINGS_CHECK_FRIENDLYDATES, BST_CHECKED);
	}

	if (m_config->showInfoTips)
	{
		CheckDlgButton(GetDialog(), IDC_OPTIONS_CHECK_SHOWINFOTIPS, BST_CHECKED);
	}

	if (m_config->infoTipType == +InfoTipType::System)
	{
		CheckDlgButton(GetDialog(), IDC_OPTIONS_RADIO_SYSTEMINFOTIPS, BST_CHECKED);
	}
	else
	{
		CheckDlgButton(GetDialog(), IDC_OPTIONS_RADIO_CUSTOMINFOTIPS, BST_CHECKED);
	}

	if (m_config->globalFolderSettings.displayMixedFilesAndFolders)
	{
		CheckDlgButton(GetDialog(), IDC_DISPLAY_MIXED_FILES_AND_FOLDERS, BST_CHECKED);
	}

	if (m_config->globalFolderSettings.useNaturalSortOrder)
	{
		CheckDlgButton(GetDialog(), IDC_USE_NATURAL_SORT_ORDER, BST_CHECKED);
	}

	AddTooltipForControl(m_tooltipWindow, GetDlgItem(GetDialog(), IDC_USE_NATURAL_SORT_ORDER),
		m_resourceLoader->LoadString(IDS_USE_NATURAL_SORT_ORDER_TOOLTIP));

	HWND fileSizesComboBox = GetDlgItem(GetDialog(), IDC_COMBO_FILESIZES);
	std::vector<ComboBoxItem> fileSizeItems;

	for (auto size : SizeDisplayFormat::_values())
	{
		if (size == +SizeDisplayFormat::None)
		{
			continue;
		}

		fileSizeItems.emplace_back(static_cast<int>(size), GetSizeDisplayFormatText(size));
	}

	AddItemsToComboBox(fileSizesComboBox, fileSizeItems,
		static_cast<int>(m_config->globalFolderSettings.sizeDisplayFormat));

	EnableWindow(fileSizesComboBox, m_config->globalFolderSettings.forceSize);

	SetInfoTipControlStates();
	SetFolderSizeControlState();
}

void FilesFoldersOptionsPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (HIWORD(wParam) != 0)
	{
		switch (HIWORD(wParam))
		{
		case EN_CHANGE:
		case CBN_SELCHANGE:
			m_settingChangedCallback();
			break;
		}
	}
	else
	{
		switch (LOWORD(wParam))
		{
		case IDC_SETTINGS_CHECK_SYSTEMFILES:
		case IDC_SETTINGS_CHECK_EXTENSIONS:
		case IDC_SETTINGS_CHECK_LINK:
		case IDC_SETTINGS_CHECK_INSERTSORTED:
		case IDC_SETTINGS_CHECK_EXISTINGFILESCONFIRMATION:
		case IDC_SETTINGS_CHECK_FOLDERSIZESNETWORKREMOVABLE:
		case IDC_SETTINGS_CHECK_CONTAINER_FILES:
		case IDC_SETTINGS_CHECK_FRIENDLYDATES:
		case IDC_OPTIONS_HOVER_TIME:
		case IDC_DISPLAY_MIXED_FILES_AND_FOLDERS:
		case IDC_USE_NATURAL_SORT_ORDER:
			m_settingChangedCallback();
			break;

		case IDC_SETTINGS_CHECK_FORCESIZE:
			EnableWindow(GetDlgItem(GetDialog(), IDC_COMBO_FILESIZES),
				IsDlgButtonChecked(GetDialog(), LOWORD(wParam)) == BST_CHECKED);
			m_settingChangedCallback();
			break;

		case IDC_OPTIONS_RADIO_SYSTEMINFOTIPS:
		case IDC_OPTIONS_RADIO_CUSTOMINFOTIPS:
			if (IsDlgButtonChecked(GetDialog(), LOWORD(wParam)) == BST_CHECKED)
			{
				m_settingChangedCallback();
			}
			break;

		case IDC_OPTIONS_CHECK_SHOWINFOTIPS:
			SetInfoTipControlStates();
			m_settingChangedCallback();
			break;

		case IDC_SETTINGS_CHECK_FOLDERSIZES:
			SetFolderSizeControlState();
			m_settingChangedCallback();
			break;

		case IDC_SETTINGS_CHECK_SINGLECLICK:
			EnableWindow(GetDlgItem(GetDialog(), IDC_OPTIONS_HOVER_TIME),
				IsDlgButtonChecked(GetDialog(), LOWORD(wParam)) == BST_CHECKED);
			EnableWindow(GetDlgItem(GetDialog(), IDC_LABEL_HOVER_TIME),
				IsDlgButtonChecked(GetDialog(), LOWORD(wParam)) == BST_CHECKED);
			m_settingChangedCallback();
			break;
		}
	}
}

void FilesFoldersOptionsPage::SetInfoTipControlStates()
{
	HWND systemInfoTipsCheckBox = GetDlgItem(GetDialog(), IDC_OPTIONS_RADIO_SYSTEMINFOTIPS);
	HWND customInfoTipsCheckBox = GetDlgItem(GetDialog(), IDC_OPTIONS_RADIO_CUSTOMINFOTIPS);

	BOOL enable = (IsDlgButtonChecked(GetDialog(), IDC_OPTIONS_CHECK_SHOWINFOTIPS) == BST_CHECKED);

	EnableWindow(systemInfoTipsCheckBox, enable);
	EnableWindow(customInfoTipsCheckBox, enable);
}

void FilesFoldersOptionsPage::SetFolderSizeControlState()
{
	HWND folderSizesNetworkRemovable =
		GetDlgItem(GetDialog(), IDC_SETTINGS_CHECK_FOLDERSIZESNETWORKREMOVABLE);

	BOOL enable = (IsDlgButtonChecked(GetDialog(), IDC_SETTINGS_CHECK_FOLDERSIZES) == BST_CHECKED);

	EnableWindow(folderSizesNetworkRemovable, enable);
}

std::wstring FilesFoldersOptionsPage::GetSizeDisplayFormatText(SizeDisplayFormat sizeDisplayFormat)
{
	UINT stringId;

	switch (sizeDisplayFormat)
	{
	case SizeDisplayFormat::Bytes:
		stringId = IDS_OPTIONS_DIALOG_FILE_SIZE_BYTES;
		break;

	case SizeDisplayFormat::KB:
		stringId = IDS_OPTIONS_DIALOG_FILE_SIZE_KB;
		break;

	case SizeDisplayFormat::MB:
		stringId = IDS_OPTIONS_DIALOG_FILE_SIZE_MB;
		break;

	case SizeDisplayFormat::GB:
		stringId = IDS_OPTIONS_DIALOG_FILE_SIZE_GB;
		break;

	case SizeDisplayFormat::TB:
		stringId = IDS_OPTIONS_DIALOG_FILE_SIZE_TB;
		break;

	case SizeDisplayFormat::PB:
		stringId = IDS_OPTIONS_DIALOG_FILE_SIZE_PB;
		break;

	// SizeDisplayFormat::None isn't an option that's displayed to the user, so there should never
	// be a string lookup for that item.
	case SizeDisplayFormat::None:
	default:
		LOG(FATAL) << "SizeDisplayFormat value not found or invalid";
		__assume(0);
	}

	return m_resourceLoader->LoadString(stringId);
}

void FilesFoldersOptionsPage::SaveSettings()
{
	HWND hCBSize;
	int iSel;

	m_config->globalFolderSettings.hideSystemFiles =
		(IsDlgButtonChecked(GetDialog(), IDC_SETTINGS_CHECK_SYSTEMFILES) == BST_CHECKED);

	m_config->globalFolderSettings.showExtensions =
		!(IsDlgButtonChecked(GetDialog(), IDC_SETTINGS_CHECK_EXTENSIONS) == BST_CHECKED);

	m_config->globalFolderSettings.hideLinkExtension =
		(IsDlgButtonChecked(GetDialog(), IDC_SETTINGS_CHECK_LINK) == BST_CHECKED);

	m_config->globalFolderSettings.insertSorted =
		(IsDlgButtonChecked(GetDialog(), IDC_SETTINGS_CHECK_INSERTSORTED) == BST_CHECKED);

	m_config->globalFolderSettings.oneClickActivate =
		(IsDlgButtonChecked(GetDialog(), IDC_SETTINGS_CHECK_SINGLECLICK) == BST_CHECKED);

	m_config->globalFolderSettings.oneClickActivateHoverTime =
		GetDlgItemInt(GetDialog(), IDC_OPTIONS_HOVER_TIME, nullptr, FALSE);

	m_config->overwriteExistingFilesConfirmation =
		(IsDlgButtonChecked(GetDialog(), IDC_SETTINGS_CHECK_EXISTINGFILESCONFIRMATION)
			== BST_CHECKED);

	m_config->globalFolderSettings.showFolderSizes =
		(IsDlgButtonChecked(GetDialog(), IDC_SETTINGS_CHECK_FOLDERSIZES) == BST_CHECKED);

	m_config->globalFolderSettings.disableFolderSizesNetworkRemovable =
		(IsDlgButtonChecked(GetDialog(), IDC_SETTINGS_CHECK_FOLDERSIZESNETWORKREMOVABLE)
			== BST_CHECKED);

	m_config->globalFolderSettings.forceSize =
		(IsDlgButtonChecked(GetDialog(), IDC_SETTINGS_CHECK_FORCESIZE) == BST_CHECKED);

	m_config->openContainerFiles =
		(IsDlgButtonChecked(GetDialog(), IDC_SETTINGS_CHECK_CONTAINER_FILES) == BST_CHECKED);

	m_config->globalFolderSettings.showFriendlyDates =
		(IsDlgButtonChecked(GetDialog(), IDC_SETTINGS_CHECK_FRIENDLYDATES) == BST_CHECKED);

	m_config->showInfoTips =
		(IsDlgButtonChecked(GetDialog(), IDC_OPTIONS_CHECK_SHOWINFOTIPS) == BST_CHECKED);

	if (IsDlgButtonChecked(GetDialog(), IDC_OPTIONS_RADIO_SYSTEMINFOTIPS) == BST_CHECKED)
	{
		m_config->infoTipType = InfoTipType::System;
	}
	else
	{
		m_config->infoTipType = InfoTipType::Custom;
	}

	m_config->globalFolderSettings.displayMixedFilesAndFolders =
		(IsDlgButtonChecked(GetDialog(), IDC_DISPLAY_MIXED_FILES_AND_FOLDERS) == BST_CHECKED);

	m_config->globalFolderSettings.useNaturalSortOrder =
		(IsDlgButtonChecked(GetDialog(), IDC_USE_NATURAL_SORT_ORDER) == BST_CHECKED);

	hCBSize = GetDlgItem(GetDialog(), IDC_COMBO_FILESIZES);

	iSel = (int) SendMessage(hCBSize, CB_GETCURSEL, 0, 0);
	m_config->globalFolderSettings.sizeDisplayFormat = SizeDisplayFormat::_from_integral(
		static_cast<SizeDisplayFormat::_integral>(SendMessage(hCBSize, CB_GETITEMDATA, iSel, 0)));

	for (auto &tab :
		m_coreInterface->GetTabContainerImpl()->GetAllTabs() | boost::adaptors::map_values)
	{
		tab->GetShellBrowserImpl()->GetNavigationController()->Refresh();
	}
}
