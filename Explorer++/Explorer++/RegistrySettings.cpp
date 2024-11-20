// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "CustomFontStorage.h"
#include "DisplayWindow/DisplayWindow.h"
#include "Storage.h"
#include "../Helper/RegistrySettings.h"
#include <wil/registry.h>

using namespace std::string_literals;

namespace
{

const wchar_t MAIN_FONT_KEY_NAME[] = L"MainFont";

}

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
			m_config->showDisplayWindow);
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

		COLORREF centreColor;
		COLORREF surroundColor;
		COLORREF textColor;
		LOGFONT logFont;
		HFONT hFont;

		centreColor = (COLORREF) SendMessage(m_displayWindow->GetHWND(), DWM_GETCENTRECOLOR, 0, 0);
		surroundColor =
			(COLORREF) SendMessage(m_displayWindow->GetHWND(), DWM_GETSURROUNDCOLOR, 0, 0);
		textColor = (COLORREF) SendMessage(m_displayWindow->GetHWND(), DWM_GETTEXTCOLOR, 0, 0);
		SendMessage(m_displayWindow->GetHWND(), DWM_GETFONT, (WPARAM) &hFont, 0);

		RegSetValueEx(hSettingsKey, _T("DisplayCentreColor"), 0, REG_BINARY, (LPBYTE) &centreColor,
			sizeof(centreColor));

		RegSetValueEx(hSettingsKey, _T("DisplaySurroundColor"), 0, REG_BINARY,
			(LPBYTE) &surroundColor, sizeof(surroundColor));

		RegSetValueEx(hSettingsKey, _T("DisplayTextColor"), 0, REG_BINARY, (LPBYTE) &textColor,
			sizeof(textColor));

		GetObject(hFont, sizeof(LOGFONT), (LPVOID) &logFont);

		RegSetValueEx(hSettingsKey, _T("DisplayFont"), 0, REG_BINARY, (LPBYTE) &logFont,
			sizeof(LOGFONT));

		auto &mainFont = m_config->mainFont.get();

		if (mainFont)
		{
			wil::unique_hkey mainFontKey;
			HRESULT hr = wil::reg::create_unique_key_nothrow(hSettingsKey, MAIN_FONT_KEY_NAME,
				mainFontKey, wil::reg::key_access::readwrite);

			if (SUCCEEDED(hr))
			{
				CustomFontStorage::SaveToRegistry(mainFontKey.get(), *mainFont);
			}
		}

		RegCloseKey(hSettingsKey);
	}

	return returnValue;
}

LONG Explorerplusplus::LoadGenericSettingsFromRegistry(HKEY applicationKey)
{
	HKEY hSettingsKey;
	LONG returnValue;
	LONG centreColorStatus = TRUE;
	LONG surroundColorStatus = TRUE;
	LONG textColorStatus = TRUE;
	LONG fontStatus = TRUE;

	returnValue = RegOpenKeyEx(applicationKey, Storage::REGISTRY_SETTINGS_KEY_NAME, 0, KEY_READ,
		&hSettingsKey);

	if (returnValue == ERROR_SUCCESS)
	{
		/* User settings. */
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("ShowExtensions"),
			m_config->globalFolderSettings.showExtensions);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("ShowStatusBar"),
			m_config->showStatusBar);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("ShowFolders"),
			m_config->showFolders);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("ShowAddressBar"),
			m_config->showAddressBar);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("ShowToolbar"),
			m_config->showMainToolbar);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("ShowBookmarksToolbar"),
			m_config->showBookmarksToolbar);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("ShowDrivesToolbar"),
			m_config->showDrivesToolbar);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("ShowApplicationToolbar"),
			m_config->showApplicationToolbar);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("AlwaysOpenNewTab"),
			m_config->alwaysOpenNewTab);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("TreeViewWidth"),
			m_config->treeViewWidth);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("ShowFriendlyDates"),
			m_config->globalFolderSettings.showFriendlyDates);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("ShowDisplayWindow"),
			m_config->showDisplayWindow);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("ShowFolderSizes"),
			m_config->globalFolderSettings.showFolderSizes);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey,
			_T("DisableFolderSizesNetworkRemovable"),
			m_config->globalFolderSettings.disableFolderSizesNetworkRemovable);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("NextToCurrent"),
			m_config->openNewTabNextToCurrent);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("ConfirmCloseTabs"),
			m_config->confirmCloseTabs);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("ShowInfoTips"),
			m_config->showInfoTips);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("TreeViewDelayEnabled"),
			m_config->treeViewDelayEnabled);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("LockToolbars"),
			m_config->lockToolbars);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("UseFullRowSelect"),
			m_config->useFullRowSelect);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("ShowFilePreviews"),
			m_config->showFilePreviews);

		RegistrySettings::ReadBetterEnumValue(hSettingsKey, _T("ReplaceExplorerMode"),
			m_config->replaceExplorerMode);

		RegistrySettings::ReadDword(hSettingsKey, _T("ShowFullTitlePath"),
			[this](DWORD value) { m_config->showFullTitlePath.set(value); });

		RegistrySettings::ReadDword(hSettingsKey, _T("ShowUserNameTitleBar"),
			[this](DWORD value) { m_config->showUserNameInTitleBar.set(value); });

		RegistrySettings::ReadDword(hSettingsKey, _T("ShowPrivilegeLevelInTitleBar"),
			[this](DWORD value) { m_config->showPrivilegeLevelInTitleBar.set(value); });

		RegistrySettings::ReadDword(hSettingsKey, _T("AlwaysShowTabBar"),
			[this](DWORD value) { m_config->alwaysShowTabBar.set(value); });

		RegistrySettings::ReadDword(hSettingsKey, _T("ShowTabBarAtBottom"),
			[this](DWORD value) { m_config->showTabBarAtBottom.set(value); });

		RegistrySettings::ReadDword(hSettingsKey, _T("ExtendTabControl"),
			[this](DWORD value) { m_config->extendTabControl.set(value); });

		RegistrySettings::ReadBetterEnumValue(hSettingsKey, _T("StartupMode"),
			m_config->startupMode);

		RegistrySettings::ReadBetterEnumValue(hSettingsKey, _T("InfoTipType"),
			m_config->infoTipType);

		RegistrySettings::ReadBetterEnumValue(hSettingsKey, _T("SizeDisplayFormat"),
			m_config->globalFolderSettings.sizeDisplayFormat);

		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("AllowMultipleInstances"),
			m_config->allowMultipleInstances);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("OneClickActivate"),
			m_config->globalFolderSettings.oneClickActivate);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("OneClickActivateHoverTime"),
			m_config->globalFolderSettings.oneClickActivateHoverTime);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("DoubleClickTabClose"),
			m_config->doubleClickTabClose);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("HandleZipFiles"),
			m_config->handleZipFiles);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("InsertSorted"),
			m_config->globalFolderSettings.insertSorted);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("CheckBoxSelection"),
			m_config->checkBoxSelection);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("ForceSize"),
			m_config->globalFolderSettings.forceSize);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("CloseMainWindowOnTabClose"),
			m_config->closeMainWindowOnTabClose);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("ShowTaskbarThumbnails"),
			m_config->showTaskbarThumbnails);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("SynchronizeTreeview"),
			m_config->synchronizeTreeview);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("TVAutoExpandSelected"),
			m_config->treeViewAutoExpandSelected);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey,
			_T("OverwriteExistingFilesConfirmation"), m_config->overwriteExistingFilesConfirmation);

		RegistrySettings::ReadDword(hSettingsKey, _T("LargeToolbarIcons"),
			[this](DWORD value) { m_config->useLargeToolbarIcons.set(value); });

		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey,
			_T("CheckPinnedToNamespaceTreeProperty"), m_config->checkPinnedToNamespaceTreeProperty);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("ShowQuickAccessInTreeView"),
			m_config->showQuickAccessInTreeView);

		auto theme = m_config->theme.get();
		RegistrySettings::ReadBetterEnumValue(hSettingsKey, _T("Theme"), theme);
		m_config->theme = theme;

		RegistrySettings::ReadString(hSettingsKey, _T("NewTabDirectory"),
			m_config->defaultTabDirectory);
		RegistrySettings::ReadBetterEnumValue(hSettingsKey, _T("IconTheme"), m_config->iconSet);

		RegistrySettings::ReadDword(hSettingsKey, _T("Language"), m_config->language);

		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("OpenTabsInForeground"),
			m_config->openTabsInForeground);

		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey,
			_T("DisplayMixedFilesAndFolders"),
			m_config->globalFolderSettings.displayMixedFilesAndFolders);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("UseNaturalSortOrder"),
			m_config->globalFolderSettings.useNaturalSortOrder);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("GoUpOnDoubleClick"),
			m_config->goUpOnDoubleClick);

		/* Global settings. */
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("ShowHiddenGlobal"),
			m_config->defaultFolderSettings.showHidden);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("ShowGridlinesGlobal"),
			m_config->globalFolderSettings.showGridlines);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("ShowInGroupsGlobal"),
			m_config->defaultFolderSettings.showInGroups);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("AutoArrangeGlobal"),
			m_config->defaultFolderSettings.autoArrange);
		RegistrySettings::ReadDword(hSettingsKey, _T("SortAscendingGlobal"),
			[this](DWORD value)
			{
				m_config->defaultFolderSettings.sortDirection =
					value ? SortDirection::Ascending : SortDirection::Descending;
				m_config->defaultFolderSettings.groupSortDirection =
					value ? SortDirection::Ascending : SortDirection::Descending;
			});
		RegistrySettings::ReadBetterEnumValue(hSettingsKey, _T("GroupSortDirectionGlobal"),
			m_config->defaultFolderSettings.groupSortDirection);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("HideSystemFilesGlobal"),
			m_config->globalFolderSettings.hideSystemFiles);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("HideLinkExtensionGlobal"),
			m_config->globalFolderSettings.hideLinkExtension);
		RegistrySettings::ReadBetterEnumValue(hSettingsKey, _T("ViewModeGlobal"),
			m_config->defaultFolderSettings.viewMode);

		/* Display window settings. */
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("DisplayWindowWidth"),
			m_config->displayWindowWidth);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("DisplayWindowHeight"),
			m_config->displayWindowHeight);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("DisplayWindowVertical"),
			m_config->displayWindowVertical);

		COLORREF centreColor;
		COLORREF surroundColor;
		COLORREF textColor;
		HFONT hFont;
		LOGFONT logFont;
		DWORD dwType;
		DWORD dwSize;

		dwType = REG_BINARY;
		dwSize = sizeof(surroundColor);

		surroundColorStatus = RegQueryValueEx(hSettingsKey, _T("DisplaySurroundColor"), nullptr,
			&dwType, (LPBYTE) &surroundColor, &dwSize);

		if (surroundColorStatus == ERROR_SUCCESS)
		{
			m_config->displayWindowSurroundColor.SetFromCOLORREF(surroundColor);
		}

		dwType = REG_BINARY;
		dwSize = sizeof(centreColor);

		centreColorStatus = RegQueryValueEx(hSettingsKey, _T("DisplayCentreColor"), nullptr,
			&dwType, (LPBYTE) &centreColor, &dwSize);

		if (centreColorStatus == ERROR_SUCCESS)
		{
			m_config->displayWindowCentreColor.SetFromCOLORREF(centreColor);
		}

		dwType = REG_BINARY;
		dwSize = sizeof(textColor);

		textColorStatus = RegQueryValueEx(hSettingsKey, _T("DisplayTextColor"), nullptr, &dwType,
			(LPBYTE) &textColor, &dwSize);

		if (textColorStatus == ERROR_SUCCESS)
		{
			m_config->displayWindowTextColor = textColor;
		}

		dwType = REG_BINARY;
		dwSize = sizeof(LOGFONT);

		fontStatus = RegQueryValueEx(hSettingsKey, _T("DisplayFont"), nullptr, &dwType,
			(LPBYTE) &logFont, &dwSize);

		if (fontStatus == ERROR_SUCCESS)
		{
			hFont = CreateFontIndirect(&logFont);

			m_config->displayWindowFont = hFont;
		}

		wil::unique_hkey mainFontKey;
		HRESULT hr = wil::reg::open_unique_key_nothrow(hSettingsKey, MAIN_FONT_KEY_NAME,
			mainFontKey, wil::reg::key_access::read);

		if (SUCCEEDED(hr))
		{
			auto mainFont = CustomFontStorage::LoadFromRegistry(mainFontKey.get());

			if (mainFont)
			{
				m_config->mainFont = *mainFont;
			}
		}

		RegCloseKey(hSettingsKey);
	}

	return returnValue;
}
