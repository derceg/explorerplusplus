// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "ConfigRegistryStorage.h"
#include "CustomFontStorage.h"
#include "Storage.h"
#include "../Helper/RegistrySettings.h"
#include <wil/registry.h>

using namespace std::string_literals;

BOOL LoadAllowMultipleInstancesFromRegistry()
{
	BOOL bAllowMultipleInstances = TRUE;

	HKEY hSettingsKey;
	auto settingsKeyPath =
		Storage::REGISTRY_APPLICATION_KEY_PATH + L"\\"s + Storage::REGISTRY_SETTINGS_KEY_NAME;
	LONG lRes =
		RegOpenKeyEx(HKEY_CURRENT_USER, settingsKeyPath.c_str(), 0, KEY_READ, &hSettingsKey);

	if (lRes == ERROR_SUCCESS)
	{
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("AllowMultipleInstances"),
			bAllowMultipleInstances);

		RegCloseKey(hSettingsKey);
	}

	return bAllowMultipleInstances;
}

LONG Explorerplusplus::SaveGenericSettingsToRegistry(HKEY applicationKey)
{
	HKEY hSettingsKey;
	DWORD disposition;
	LONG returnValue;

	returnValue = RegCreateKeyEx(applicationKey, Storage::REGISTRY_SETTINGS_KEY_NAME, 0, nullptr,
		REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hSettingsKey, &disposition);

	if (returnValue == ERROR_SUCCESS)
	{
		/* User settings. */
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowExtensions"),
			m_config->globalFolderSettings.showExtensions);
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowStatusBar"), m_config->showStatusBar);
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowFolders"), m_config->showFolders.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowAddressBar"),
			m_config->showAddressBar.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowToolbar"),
			m_config->showMainToolbar.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowBookmarksToolbar"),
			m_config->showBookmarksToolbar.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowDrivesToolbar"),
			m_config->showDrivesToolbar.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowApplicationToolbar"),
			m_config->showApplicationToolbar.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowFullTitlePath"),
			m_config->showFullTitlePath.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("AlwaysOpenNewTab"),
			m_config->alwaysOpenNewTab);
		RegistrySettings::SaveDword(hSettingsKey, _T("TreeViewWidth"), m_config->treeViewWidth);
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowFriendlyDates"),
			m_config->globalFolderSettings.showFriendlyDates);
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowDisplayWindow"),
			m_config->showDisplayWindow.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowFolderSizes"),
			m_config->globalFolderSettings.showFolderSizes);
		RegistrySettings::SaveDword(hSettingsKey, _T("DisableFolderSizesNetworkRemovable"),
			m_config->globalFolderSettings.disableFolderSizesNetworkRemovable);
		RegistrySettings::SaveDword(hSettingsKey, _T("StartupMode"), m_config->startupMode);
		RegistrySettings::SaveDword(hSettingsKey, _T("NextToCurrent"),
			m_config->openNewTabNextToCurrent);
		RegistrySettings::SaveDword(hSettingsKey, _T("ConfirmCloseTabs"),
			m_config->confirmCloseTabs);
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowInfoTips"), m_config->showInfoTips);
		RegistrySettings::SaveDword(hSettingsKey, _T("InfoTipType"), m_config->infoTipType);
		RegistrySettings::SaveDword(hSettingsKey, _T("TreeViewDelayEnabled"),
			m_config->treeViewDelayEnabled);
		RegistrySettings::SaveDword(hSettingsKey, _T("LockToolbars"), m_config->lockToolbars.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("ExtendTabControl"),
			m_config->extendTabControl.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("UseFullRowSelect"),
			m_config->useFullRowSelect.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowFilePreviews"),
			m_config->showFilePreviews);
		RegistrySettings::SaveDword(hSettingsKey, _T("ReplaceExplorerMode"),
			m_config->replaceExplorerMode);
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowUserNameTitleBar"),
			m_config->showUserNameInTitleBar.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("AllowMultipleInstances"),
			m_config->allowMultipleInstances);
		RegistrySettings::SaveDword(hSettingsKey, _T("OneClickActivate"),
			m_config->globalFolderSettings.oneClickActivate.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("OneClickActivateHoverTime"),
			m_config->globalFolderSettings.oneClickActivateHoverTime.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("DoubleClickTabClose"),
			m_config->doubleClickTabClose);
		RegistrySettings::SaveDword(hSettingsKey, _T("HandleZipFiles"), m_config->handleZipFiles);
		RegistrySettings::SaveDword(hSettingsKey, _T("InsertSorted"),
			m_config->globalFolderSettings.insertSorted);
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowPrivilegeLevelInTitleBar"),
			m_config->showPrivilegeLevelInTitleBar.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("AlwaysShowTabBar"),
			m_config->alwaysShowTabBar.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("CheckBoxSelection"),
			m_config->checkBoxSelection.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("ForceSize"),
			m_config->globalFolderSettings.forceSize);
		RegistrySettings::SaveDword(hSettingsKey, _T("SizeDisplayFormat"),
			m_config->globalFolderSettings.sizeDisplayFormat);
		RegistrySettings::SaveDword(hSettingsKey, _T("CloseMainWindowOnTabClose"),
			m_config->closeMainWindowOnTabClose);
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowTabBarAtBottom"),
			m_config->showTabBarAtBottom.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("OverwriteExistingFilesConfirmation"),
			m_config->overwriteExistingFilesConfirmation);
		RegistrySettings::SaveDword(hSettingsKey, _T("LargeToolbarIcons"),
			m_config->useLargeToolbarIcons.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("CheckPinnedToNamespaceTreeProperty"),
			m_config->checkPinnedToNamespaceTreeProperty);
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowQuickAccessInTreeView"),
			m_config->showQuickAccessInTreeView.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("Theme"), m_config->theme.get());

		RegistrySettings::SaveString(hSettingsKey, _T("NewTabDirectory"),
			m_config->defaultTabDirectory);

		RegistrySettings::SaveDword(hSettingsKey, _T("IconTheme"), m_config->iconSet);
		RegistrySettings::SaveDword(hSettingsKey, _T("Language"), m_config->language);
		RegistrySettings::SaveDword(hSettingsKey, _T("OpenTabsInForeground"),
			m_config->openTabsInForeground);

		RegistrySettings::SaveDword(hSettingsKey, _T("DisplayMixedFilesAndFolders"),
			m_config->globalFolderSettings.displayMixedFilesAndFolders);
		RegistrySettings::SaveDword(hSettingsKey, _T("UseNaturalSortOrder"),
			m_config->globalFolderSettings.useNaturalSortOrder);
		RegistrySettings::SaveDword(hSettingsKey, _T("GoUpOnDoubleClick"),
			m_config->goUpOnDoubleClick);

		/* Global settings. */
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowHiddenGlobal"),
			m_config->defaultFolderSettings.showHidden);
		RegistrySettings::SaveDword(hSettingsKey, _T("ViewModeGlobal"),
			m_config->defaultFolderSettings.viewMode);
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowGridlinesGlobal"),
			m_config->globalFolderSettings.showGridlines.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowInGroupsGlobal"),
			m_config->defaultFolderSettings.showInGroups);
		RegistrySettings::SaveDword(hSettingsKey, _T("AutoArrangeGlobal"),
			m_config->defaultFolderSettings.autoArrange);
		RegistrySettings::SaveDword(hSettingsKey, _T("SortAscendingGlobal"),
			m_config->defaultFolderSettings.sortDirection == +SortDirection::Ascending);
		RegistrySettings::SaveDword(hSettingsKey, _T("GroupSortDirectionGlobal"),
			m_config->defaultFolderSettings.groupSortDirection);
		RegistrySettings::SaveDword(hSettingsKey, _T("HideSystemFilesGlobal"),
			m_config->globalFolderSettings.hideSystemFiles);
		RegistrySettings::SaveDword(hSettingsKey, _T("HideLinkExtensionGlobal"),
			m_config->globalFolderSettings.hideLinkExtension);
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowTaskbarThumbnails"),
			m_config->showTaskbarThumbnails);
		RegistrySettings::SaveDword(hSettingsKey, _T("SynchronizeTreeview"),
			m_config->synchronizeTreeview.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("TVAutoExpandSelected"),
			m_config->treeViewAutoExpandSelected);

		/* Display window settings. */
		RegistrySettings::SaveDword(hSettingsKey, _T("DisplayWindowWidth"),
			m_config->displayWindowWidth);
		RegistrySettings::SaveDword(hSettingsKey, _T("DisplayWindowHeight"),
			m_config->displayWindowHeight);
		RegistrySettings::SaveDword(hSettingsKey, _T("DisplayWindowVertical"),
			m_config->displayWindowVertical);

		RegSetValueEx(hSettingsKey, _T("DisplayCentreColor"), 0, REG_BINARY,
			(LPBYTE) &m_config->displayWindowCentreColor.get(),
			sizeof(m_config->displayWindowCentreColor.get()));
		RegSetValueEx(hSettingsKey, _T("DisplaySurroundColor"), 0, REG_BINARY,
			(LPBYTE) &m_config->displayWindowSurroundColor.get(),
			sizeof(m_config->displayWindowSurroundColor.get()));
		RegSetValueEx(hSettingsKey, _T("DisplayTextColor"), 0, REG_BINARY,
			(LPBYTE) &m_config->displayWindowTextColor.get(),
			sizeof(m_config->displayWindowTextColor.get()));
		RegSetValueEx(hSettingsKey, _T("DisplayFont"), 0, REG_BINARY,
			(LPBYTE) &m_config->displayWindowFont.get(), sizeof(m_config->displayWindowFont.get()));

		auto &mainFont = m_config->mainFont.get();

		if (mainFont)
		{
			wil::unique_hkey mainFontKey;
			HRESULT hr = wil::reg::create_unique_key_nothrow(hSettingsKey,
				ConfigRegistryStorage::MAIN_FONT_KEY_NAME, mainFontKey,
				wil::reg::key_access::readwrite);

			if (SUCCEEDED(hr))
			{
				CustomFontStorage::SaveToRegistry(mainFontKey.get(), *mainFont);
			}
		}

		RegCloseKey(hSettingsKey);
	}

	return returnValue;
}
