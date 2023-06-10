// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "DefaultColumns.h"
#include "DisplayWindow/DisplayWindow.h"
#include "Explorer++_internal.h"
#include "MainToolbar.h"
#include "ShellBrowser/ShellBrowser.h"
#include "TabContainer.h"
#include "../Helper/Macros.h"
#include "../Helper/RegistrySettings.h"

namespace
{
const TCHAR REG_TABS_KEY[] = _T("Software\\Explorer++\\Tabs");
const TCHAR REG_TOOLBARS_KEY[] = _T("Software\\Explorer++\\Toolbars");
const TCHAR REG_COLUMNS_KEY[] = _T("Software\\Explorer++\\DefaultColumns");
}

void UpdateColumnWidths(std::vector<Column_t> &columns,
	const std::vector<ColumnWidth> &columnWidths);

BOOL LoadWindowPositionFromRegistry(WINDOWPLACEMENT *pwndpl)
{
	HKEY hSettingsKey;
	BOOL bRes = FALSE;

	LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER, NExplorerplusplus::REG_SETTINGS_KEY, 0, KEY_READ,
		&hSettingsKey);

	if (lRes == ERROR_SUCCESS)
	{
		DWORD dwSize = sizeof(WINDOWPLACEMENT);

		RegQueryValueEx(hSettingsKey, _T("Position"), nullptr, nullptr, (LPBYTE) pwndpl, &dwSize);

		if (dwSize == sizeof(WINDOWPLACEMENT) && pwndpl->length == sizeof(WINDOWPLACEMENT))
		{
			bRes = TRUE;
		}

		RegCloseKey(hSettingsKey);
	}

	return bRes;
}

BOOL LoadAllowMultipleInstancesFromRegistry()
{
	BOOL bAllowMultipleInstances = TRUE;

	HKEY hSettingsKey;
	LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER, NExplorerplusplus::REG_SETTINGS_KEY, 0, KEY_READ,
		&hSettingsKey);

	if (lRes == ERROR_SUCCESS)
	{
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("AllowMultipleInstances"),
			bAllowMultipleInstances);

		RegCloseKey(hSettingsKey);
	}

	return bAllowMultipleInstances;
}

LONG Explorerplusplus::SaveGenericSettingsToRegistry()
{
	HKEY hSettingsKey;
	DWORD disposition;
	LONG returnValue;
	TBSAVEPARAMS tbSave;

	/* Open/Create the main key that is used to store data. */
	returnValue = RegCreateKeyEx(HKEY_CURRENT_USER, NExplorerplusplus::REG_SETTINGS_KEY, 0, nullptr,
		REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hSettingsKey, &disposition);

	if (returnValue == ERROR_SUCCESS)
	{
		WINDOWPLACEMENT wndpl;

		wndpl.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(m_hContainer, &wndpl);

		/* Window position. */
		RegSetValueEx(hSettingsKey, _T("Position"), 0, REG_BINARY, (LPBYTE) &wndpl, sizeof(wndpl));

		/* User settings. */
		RegistrySettings::SaveDword(hSettingsKey, _T("LastSelectedTab"), m_iLastSelectedTab);
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowExtensions"),
			m_config->globalFolderSettings.showExtensions);
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowStatusBar"), m_config->showStatusBar);
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowFolders"), m_config->showFolders.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowAddressBar"), m_config->showAddressBar);
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowToolbar"), m_config->showMainToolbar);
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowBookmarksToolbar"),
			m_config->showBookmarksToolbar);
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowDrivesToolbar"),
			m_config->showDrivesToolbar);
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowApplicationToolbar"),
			m_config->showApplicationToolbar);
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
		RegistrySettings::SaveDword(hSettingsKey, _T("StartupMode"),
			static_cast<DWORD>(m_config->startupMode));
		RegistrySettings::SaveDword(hSettingsKey, _T("NextToCurrent"),
			m_config->openNewTabNextToCurrent);
		RegistrySettings::SaveDword(hSettingsKey, _T("ConfirmCloseTabs"),
			m_config->confirmCloseTabs);
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowInfoTips"), m_config->showInfoTips);
		RegistrySettings::SaveDword(hSettingsKey, _T("InfoTipType"),
			static_cast<DWORD>(m_config->infoTipType));
		RegistrySettings::SaveDword(hSettingsKey, _T("TreeViewDelayEnabled"),
			m_config->treeViewDelayEnabled);
		RegistrySettings::SaveDword(hSettingsKey, _T("LockToolbars"), m_config->lockToolbars);
		RegistrySettings::SaveDword(hSettingsKey, _T("ExtendTabControl"),
			m_config->extendTabControl.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("UseFullRowSelect"),
			m_config->useFullRowSelect.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowFilePreviews"),
			m_config->showFilePreviews);
		RegistrySettings::SaveDword(hSettingsKey, _T("ReplaceExplorerMode"),
			static_cast<DWORD>(m_config->replaceExplorerMode));
		RegistrySettings::SaveDword(hSettingsKey, _T("ShowUserNameTitleBar"),
			m_config->showUserNameInTitleBar.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("AllowMultipleInstances"),
			m_config->allowMultipleInstances);
		RegistrySettings::SaveDword(hSettingsKey, _T("OneClickActivate"),
			m_config->globalFolderSettings.oneClickActivate.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("OneClickActivateHoverTime"),
			m_config->globalFolderSettings.oneClickActivateHoverTime.get());
		RegistrySettings::SaveDword(hSettingsKey, _T("ForceSameTabWidth"),
			m_config->forceSameTabWidth.get());
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
			static_cast<DWORD>(m_config->globalFolderSettings.sizeDisplayFormat));
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
			m_config->synchronizeTreeview);
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

		centreColor = (COLORREF) SendMessage(m_hDisplayWindow, DWM_GETCENTRECOLOR, 0, 0);
		surroundColor = (COLORREF) SendMessage(m_hDisplayWindow, DWM_GETSURROUNDCOLOR, 0, 0);
		textColor = (COLORREF) SendMessage(m_hDisplayWindow, DWM_GETTEXTCOLOR, 0, 0);
		SendMessage(m_hDisplayWindow, DWM_GETFONT, (WPARAM) &hFont, 0);

		RegSetValueEx(hSettingsKey, _T("DisplayCentreColor"), 0, REG_BINARY, (LPBYTE) &centreColor,
			sizeof(centreColor));

		RegSetValueEx(hSettingsKey, _T("DisplaySurroundColor"), 0, REG_BINARY,
			(LPBYTE) &surroundColor, sizeof(surroundColor));

		RegSetValueEx(hSettingsKey, _T("DisplayTextColor"), 0, REG_BINARY, (LPBYTE) &textColor,
			sizeof(textColor));

		GetObject(hFont, sizeof(LOGFONT), (LPVOID) &logFont);

		RegSetValueEx(hSettingsKey, _T("DisplayFont"), 0, REG_BINARY, (LPBYTE) &logFont,
			sizeof(LOGFONT));

		/* TODO: This should
		be done within the
		main toolbar class. */
		tbSave.hkr = HKEY_CURRENT_USER;
		tbSave.pszSubKey = NExplorerplusplus::REG_SETTINGS_KEY;
		tbSave.pszValueName = _T("ToolbarState");

		SendMessage(m_mainToolbar->GetHWND(), TB_SAVERESTORE, TRUE, (LPARAM) &tbSave);

		RegCloseKey(hSettingsKey);
	}

	return returnValue;
}

LONG Explorerplusplus::LoadGenericSettingsFromRegistry()
{
	HKEY hSettingsKey;
	LONG returnValue;
	LONG centreColorStatus = TRUE;
	LONG surroundColorStatus = TRUE;
	LONG textColorStatus = TRUE;
	LONG fontStatus = TRUE;

	/* Open/Create the main key that is used to store data. */
	returnValue = RegOpenKeyEx(HKEY_CURRENT_USER, NExplorerplusplus::REG_SETTINGS_KEY, 0, KEY_READ,
		&hSettingsKey);

	if (returnValue == ERROR_SUCCESS)
	{
		/* User settings. */
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("LastSelectedTab"),
			m_iLastSelectedTab);
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

		RegistrySettings::ReadDword(hSettingsKey, _T("ReplaceExplorerMode"),
			[this](DWORD value) {
				m_config->replaceExplorerMode =
					static_cast<DefaultFileManager::ReplaceExplorerMode>(value);
			});

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

		RegistrySettings::ReadDword(hSettingsKey, _T("ForceSameTabWidth"),
			[this](DWORD value) { m_config->forceSameTabWidth.set(value); });

		RegistrySettings::ReadDword(hSettingsKey, _T("StartupMode"),
			[this](DWORD value) { m_config->startupMode = static_cast<StartupMode>(value); });

		RegistrySettings::ReadDword(hSettingsKey, _T("InfoTipType"),
			[this](DWORD value) { m_config->infoTipType = static_cast<InfoTipType>(value); });

		RegistrySettings::ReadDword(hSettingsKey, _T("SizeDisplayFormat"),
			[this](DWORD value) {
				m_config->globalFolderSettings.sizeDisplayFormat =
					static_cast<SizeDisplayFormat>(value);
			});

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

		bool themeLoaded = false;
		RegistrySettings::ReadDword(hSettingsKey, _T("Theme"),
			[this, &themeLoaded](DWORD value)
			{
				m_config->theme = Theme::_from_integral(value);
				themeLoaded = true;
			});

		if (!themeLoaded)
		{
			// Theme data was previously stored using this key.
			RegistrySettings::ReadDword(hSettingsKey, _T("EnableDarkMode"),
				[this](DWORD value) { m_config->theme = value ? Theme::Dark : Theme::Light; });
		}

		RegistrySettings::ReadString(hSettingsKey, _T("NewTabDirectory"),
			m_config->defaultTabDirectory);

		RegistrySettings::ReadDword(hSettingsKey, _T("IconTheme"),
			[this](DWORD value) { m_config->iconSet = IconSet::_from_integral(value); });

		RegistrySettings::ReadDword(hSettingsKey, _T("Language"),
			[this](DWORD value)
			{
				m_config->language = value;
				m_bLanguageLoaded = true;
			});

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
		RegistrySettings::ReadDword(hSettingsKey, _T("GroupSortDirectionGlobal"),
			[this](DWORD value) {
				m_config->defaultFolderSettings.groupSortDirection =
					SortDirection::_from_integral(value);
			});
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("HideSystemFilesGlobal"),
			m_config->globalFolderSettings.hideSystemFiles);
		RegistrySettings::Read32BitValueFromRegistry(hSettingsKey, _T("HideLinkExtensionGlobal"),
			m_config->globalFolderSettings.hideLinkExtension);

		RegistrySettings::ReadDword(hSettingsKey, _T("ViewModeGlobal"),
			[this](DWORD value)
			{ m_config->defaultFolderSettings.viewMode = ViewMode::_from_integral(value); });

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

		m_bAttemptToolbarRestore = true;

		RegCloseKey(hSettingsKey);
	}

	return returnValue;
}

void Explorerplusplus::SaveTabSettingsToRegistry()
{
	HKEY hKey;
	HKEY hTabKey;
	HKEY hColumnsKey;
	TCHAR szItemKey[128];
	DWORD disposition;
	LONG returnValue;

	/* First, delete all current tab keys. If these keys
	are not deleted beforehand, then they may be opened
	again on the next program run, even when they were
	closed. */
	SHDeleteKey(HKEY_CURRENT_USER, REG_TABS_KEY);

	returnValue = RegCreateKeyEx(HKEY_CURRENT_USER, REG_TABS_KEY, 0, nullptr,
		REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, &disposition);

	if (returnValue == ERROR_SUCCESS)
	{
		int tabNum = 0;

		for (auto tabRef : m_tabContainer->GetAllTabsInOrder())
		{
			auto &tab = tabRef.get();

			StringCchPrintf(szItemKey, SIZEOF_ARRAY(szItemKey), _T("%d"), tabNum);

			returnValue = RegCreateKeyEx(hKey, szItemKey, 0, nullptr, REG_OPTION_NON_VOLATILE,
				KEY_WRITE, nullptr, &hTabKey, &disposition);

			if (returnValue == ERROR_SUCCESS)
			{
				auto pidlDirectory = tab.GetShellBrowser()->GetDirectoryIdl();
				RegSetValueEx(hTabKey, _T("Directory"), 0, REG_BINARY, (LPBYTE) pidlDirectory.get(),
					ILGetSize(pidlDirectory.get()));

				RegistrySettings::SaveDword(hTabKey, _T("ViewMode"),
					tab.GetShellBrowser()->GetViewMode());
				RegistrySettings::SaveDword(hTabKey, _T("SortMode"),
					tab.GetShellBrowser()->GetSortMode());

				// For backwards compatibility, the value saved here is a bool.
				RegistrySettings::SaveDword(hTabKey, _T("SortAscending"),
					tab.GetShellBrowser()->GetSortDirection() == +SortDirection::Ascending);

				RegistrySettings::SaveDword(hTabKey, _T("GroupMode"),
					tab.GetShellBrowser()->GetGroupMode());
				RegistrySettings::SaveDword(hTabKey, _T("GroupSortDirection"),
					tab.GetShellBrowser()->GetGroupSortDirection());
				RegistrySettings::SaveDword(hTabKey, _T("ShowInGroups"),
					tab.GetShellBrowser()->GetShowInGroups());
				RegistrySettings::SaveDword(hTabKey, _T("ApplyFilter"),
					tab.GetShellBrowser()->IsFilterApplied());
				RegistrySettings::SaveDword(hTabKey, _T("FilterCaseSensitive"),
					tab.GetShellBrowser()->GetFilterCaseSensitive());
				RegistrySettings::SaveDword(hTabKey, _T("ShowHidden"),
					tab.GetShellBrowser()->GetShowHidden());
				RegistrySettings::SaveDword(hTabKey, _T("AutoArrange"),
					tab.GetShellBrowser()->GetAutoArrange());

				std::wstring filter = tab.GetShellBrowser()->GetFilterText();
				RegistrySettings::SaveString(hTabKey, _T("Filter"), filter);

				/* Now save the tabs columns. */
				returnValue = RegCreateKeyEx(hTabKey, _T("Columns"), 0, nullptr,
					REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hColumnsKey, &disposition);

				if (returnValue == ERROR_SUCCESS)
				{
					FolderColumns folderColumns = tab.GetShellBrowser()->ExportAllColumns();

					SaveColumnToRegistry(hColumnsKey, _T("ControlPanelColumns"),
						&folderColumns.controlPanelColumns);
					SaveColumnToRegistry(hColumnsKey, _T("MyComputerColumns"),
						&folderColumns.myComputerColumns);
					SaveColumnToRegistry(hColumnsKey, _T("RealFolderColumns"),
						&folderColumns.realFolderColumns);
					SaveColumnToRegistry(hColumnsKey, _T("RecycleBinColumns"),
						&folderColumns.recycleBinColumns);
					SaveColumnToRegistry(hColumnsKey, _T("PrinterColumns"),
						&folderColumns.printersColumns);
					SaveColumnToRegistry(hColumnsKey, _T("NetworkColumns"),
						&folderColumns.networkConnectionsColumns);
					SaveColumnToRegistry(hColumnsKey, _T("NetworkPlacesColumns"),
						&folderColumns.myNetworkPlacesColumns);

					/* Now save column widths. In the future, these keys may be merged with
					the column keys above. */
					SaveColumnWidthsToRegistry(hColumnsKey, _T("ControlPanelColumnWidths"),
						&folderColumns.controlPanelColumns);
					SaveColumnWidthsToRegistry(hColumnsKey, _T("MyComputerColumnWidths"),
						&folderColumns.myComputerColumns);
					SaveColumnWidthsToRegistry(hColumnsKey, _T("RealFolderColumnWidths"),
						&folderColumns.realFolderColumns);
					SaveColumnWidthsToRegistry(hColumnsKey, _T("RecycleBinColumnWidths"),
						&folderColumns.recycleBinColumns);
					SaveColumnWidthsToRegistry(hColumnsKey, _T("PrinterColumnWidths"),
						&folderColumns.printersColumns);
					SaveColumnWidthsToRegistry(hColumnsKey, _T("NetworkColumnWidths"),
						&folderColumns.networkConnectionsColumns);
					SaveColumnWidthsToRegistry(hColumnsKey, _T("NetworkPlacesColumnWidths"),
						&folderColumns.myNetworkPlacesColumns);

					RegCloseKey(hColumnsKey);
				}

				/* High-level settings. */
				RegistrySettings::SaveDword(hTabKey, _T("Locked"),
					tab.GetLockState() == Tab::LockState::Locked);
				RegistrySettings::SaveDword(hTabKey, _T("AddressLocked"),
					tab.GetLockState() == Tab::LockState::AddressLocked);
				RegistrySettings::SaveDword(hTabKey, _T("UseCustomName"), tab.GetUseCustomName());

				if (tab.GetUseCustomName())
				{
					RegistrySettings::SaveString(hTabKey, _T("CustomName"), tab.GetName());
				}
				else
				{
					RegistrySettings::SaveString(hTabKey, _T("CustomName"), L"");
				}

				RegCloseKey(hTabKey);
			}

			tabNum++;
		}

		RegCloseKey(hKey);
	}
}

void UpdateColumnWidths(std::vector<Column_t> &columns,
	const std::vector<ColumnWidth> &columnWidths)
{
	for (auto itr1 = columnWidths.begin(); itr1 != columnWidths.end(); itr1++)
	{
		for (auto itr2 = columns.begin(); itr2 != columns.end(); itr2++)
		{
			if (static_cast<unsigned int>(itr2->type) == itr1->id)
			{
				itr2->iWidth = itr1->iWidth;
				break;
			}
		}
	}
}

int Explorerplusplus::LoadTabSettingsFromRegistry()
{
	HKEY hKey;
	HKEY hTabKey;
	HKEY hColumnsKey;
	TCHAR szItemKey[128];
	PIDLIST_ABSOLUTE pidlDirectory = nullptr;
	LONG returnValue;
	DWORD cbData;
	DWORD type;
	int nTabsCreated = 0;
	int i = 0;

	returnValue = RegOpenKeyEx(HKEY_CURRENT_USER, REG_TABS_KEY, 0, KEY_READ, &hKey);

	if (returnValue == ERROR_SUCCESS)
	{
		StringCchPrintf(szItemKey, SIZEOF_ARRAY(szItemKey), _T("%d"), i);

		returnValue = RegOpenKeyEx(hKey, szItemKey, 0, KEY_READ, &hTabKey);

		while (returnValue == ERROR_SUCCESS)
		{
			if (RegQueryValueEx(hTabKey, _T("Directory"), nullptr, nullptr, nullptr, &cbData)
				== ERROR_SUCCESS)
			{
				pidlDirectory = (PIDLIST_ABSOLUTE) CoTaskMemAlloc(cbData);

				RegQueryValueEx(hTabKey, _T("Directory"), nullptr, &type, (LPBYTE) pidlDirectory,
					&cbData);
			}

			FolderSettings folderSettings;

			RegistrySettings::ReadDword(hTabKey, _T("ViewMode"),
				[&folderSettings](DWORD value)
				{ folderSettings.viewMode = ViewMode::_from_integral(value); });
			RegistrySettings::ReadDword(hTabKey, _T("SortMode"),
				[&folderSettings](DWORD value)
				{
					folderSettings.sortMode = SortMode::_from_integral(value);

					// Previously, the group mode and sort mode were always the same. Therefore, the
					// group mode is set here to preserve the behavior. This will only have an
					// effect when updating from a version that didn't save a separate group mode.
					// If a group mode has been saved, it will be loaded below and the value loaded
					// here will be overwritten.
					folderSettings.groupMode = SortMode::_from_integral(value);
				});
			RegistrySettings::ReadDword(hTabKey, _T("SortAscending"),
				[&folderSettings](DWORD value)
				{
					folderSettings.sortDirection =
						value ? SortDirection::Ascending : SortDirection::Descending;

					// As with the group mode/sort mode, the group sort direction and standard sort
					// direction were always the same in previous versions.
					folderSettings.groupSortDirection =
						value ? SortDirection::Ascending : SortDirection::Descending;
				});
			RegistrySettings::ReadDword(hTabKey, _T("GroupMode"),
				[&folderSettings](DWORD value)
				{ folderSettings.groupMode = SortMode::_from_integral(value); });
			RegistrySettings::ReadDword(hTabKey, _T("GroupSortDirection"),
				[&folderSettings](DWORD value)
				{ folderSettings.groupSortDirection = SortDirection::_from_integral(value); });
			RegistrySettings::Read32BitValueFromRegistry(hTabKey, _T("ShowInGroups"),
				folderSettings.showInGroups);
			RegistrySettings::Read32BitValueFromRegistry(hTabKey, _T("ApplyFilter"),
				folderSettings.applyFilter);
			RegistrySettings::Read32BitValueFromRegistry(hTabKey, _T("FilterCaseSensitive"),
				folderSettings.filterCaseSensitive);
			RegistrySettings::Read32BitValueFromRegistry(hTabKey, _T("ShowHidden"),
				folderSettings.showHidden);
			RegistrySettings::Read32BitValueFromRegistry(hTabKey, _T("AutoArrange"),
				folderSettings.autoArrange);

			RegistrySettings::ReadString(hTabKey, _T("Filter"), folderSettings.filter);

			/* Now load this tabs columns. */
			returnValue = RegOpenKeyEx(hTabKey, _T("Columns"), 0, KEY_READ, &hColumnsKey);

			FolderColumns initialColumns;

			if (returnValue == ERROR_SUCCESS)
			{
				initialColumns.controlPanelColumns =
					LoadColumnFromRegistry(hColumnsKey, _T("ControlPanelColumns"));
				initialColumns.myComputerColumns =
					LoadColumnFromRegistry(hColumnsKey, _T("MyComputerColumns"));
				initialColumns.realFolderColumns =
					LoadColumnFromRegistry(hColumnsKey, _T("RealFolderColumns"));
				initialColumns.recycleBinColumns =
					LoadColumnFromRegistry(hColumnsKey, _T("RecycleBinColumns"));
				initialColumns.printersColumns =
					LoadColumnFromRegistry(hColumnsKey, _T("PrinterColumns"));
				initialColumns.networkConnectionsColumns =
					LoadColumnFromRegistry(hColumnsKey, _T("NetworkColumns"));
				initialColumns.myNetworkPlacesColumns =
					LoadColumnFromRegistry(hColumnsKey, _T("NetworkPlacesColumns"));

				auto controlPanelWidths =
					LoadColumnWidthsFromRegistry(hColumnsKey, _T("ControlPanelColumnWidths"));
				auto myComputerWidths =
					LoadColumnWidthsFromRegistry(hColumnsKey, _T("MyComputerColumnWidths"));
				auto realFolderWidths =
					LoadColumnWidthsFromRegistry(hColumnsKey, _T("RealFolderColumnWidths"));
				auto recycleBinWidths =
					LoadColumnWidthsFromRegistry(hColumnsKey, _T("RecycleBinColumnWidths"));
				auto printersWidths =
					LoadColumnWidthsFromRegistry(hColumnsKey, _T("PrinterColumnWidths"));
				auto networkConnectionsWidths =
					LoadColumnWidthsFromRegistry(hColumnsKey, _T("NetworkColumnWidths"));
				auto myNetworkPlacesWidths =
					LoadColumnWidthsFromRegistry(hColumnsKey, _T("NetworkPlacesColumnWidths"));

				UpdateColumnWidths(initialColumns.controlPanelColumns, controlPanelWidths);
				UpdateColumnWidths(initialColumns.myComputerColumns, myComputerWidths);
				UpdateColumnWidths(initialColumns.realFolderColumns, realFolderWidths);
				UpdateColumnWidths(initialColumns.recycleBinColumns, recycleBinWidths);
				UpdateColumnWidths(initialColumns.printersColumns, printersWidths);
				UpdateColumnWidths(initialColumns.networkConnectionsColumns,
					networkConnectionsWidths);
				UpdateColumnWidths(initialColumns.myNetworkPlacesColumns, myNetworkPlacesWidths);

				RegCloseKey(hColumnsKey);
			}

			ValidateColumns(initialColumns);

			TabSettings tabSettings;

			tabSettings.index = i;
			tabSettings.selected = true;

			RegistrySettings::ReadDword(hTabKey, _T("Locked"),
				[&tabSettings](DWORD value)
				{
					if (value)
					{
						tabSettings.lockState = Tab::LockState::Locked;
					}
				});

			RegistrySettings::ReadDword(hTabKey, _T("AddressLocked"),
				[&tabSettings](DWORD value)
				{
					if (value)
					{
						tabSettings.lockState = Tab::LockState::AddressLocked;
					}
				});

			std::wstring customName;
			LSTATUS result = RegistrySettings::ReadString(hTabKey, _T("CustomName"), customName);

			if (result == ERROR_SUCCESS)
			{
				tabSettings.name = customName;
			}

			auto navigateParams = NavigateParams::Normal(pidlDirectory);
			m_tabContainer->CreateNewTab(navigateParams, tabSettings, &folderSettings,
				&initialColumns);

			nTabsCreated++;

			CoTaskMemFree(pidlDirectory);
			RegCloseKey(hTabKey);

			i++;

			StringCchPrintf(szItemKey, SIZEOF_ARRAY(szItemKey), _T("%d"), i);

			returnValue = RegOpenKeyEx(hKey, szItemKey, 0, KEY_READ, &hTabKey);
		}

		RegCloseKey(hKey);
	}

	return nTabsCreated;
}

void Explorerplusplus::SaveColumnWidthsToRegistry(HKEY hColumnsKey, const TCHAR *szKeyName,
	std::vector<Column_t> *pColumns)
{
	ColumnWidth *pColumnList = nullptr;
	int iColumn = 0;

	pColumnList = (ColumnWidth *) malloc(pColumns->size() * sizeof(ColumnWidth));

	for (auto itr = pColumns->begin(); itr != pColumns->end(); itr++)
	{
		pColumnList[iColumn].id = static_cast<unsigned int>(itr->type);
		pColumnList[iColumn].iWidth = itr->iWidth;

		iColumn++;
	}

	RegSetValueEx(hColumnsKey, szKeyName, 0, REG_BINARY, (LPBYTE) pColumnList,
		(DWORD) (pColumns->size() * sizeof(ColumnWidth)));

	free(pColumnList);
}

std::vector<ColumnWidth> Explorerplusplus::LoadColumnWidthsFromRegistry(HKEY hColumnsKey,
	const TCHAR *szKeyName)
{
	ColumnWidth columnWidthData[64];
	DWORD dwType = REG_BINARY;
	DWORD dwSize = sizeof(columnWidthData);

	LONG ret = RegQueryValueEx(hColumnsKey, szKeyName, nullptr, &dwType, (LPBYTE) columnWidthData,
		&dwSize);

	std::vector<ColumnWidth> columnWidths;

	if (ret == ERROR_SUCCESS)
	{
		for (unsigned int i = 0; i < dwSize / sizeof(ColumnWidth); i++)
		{
			ColumnWidth columnWidth;
			columnWidth.id = columnWidthData[i].id;
			columnWidth.iWidth = columnWidthData[i].iWidth;

			columnWidths.push_back(columnWidth);
		}
	}

	return columnWidths;
}

void Explorerplusplus::SaveColumnToRegistry(HKEY hColumnsKey, const TCHAR *szKeyName,
	std::vector<Column_t> *pColumns)
{
	ColumnOld_t *pColumnList = nullptr;
	int iColumn = 0;

	pColumnList = (ColumnOld_t *) malloc(pColumns->size() * sizeof(ColumnOld_t));

	for (auto itr = pColumns->begin(); itr != pColumns->end(); itr++)
	{
		pColumnList[iColumn].id = static_cast<unsigned int>(itr->type);
		pColumnList[iColumn].bChecked = itr->bChecked;

		iColumn++;
	}

	RegSetValueEx(hColumnsKey, szKeyName, 0, REG_BINARY, (LPBYTE) pColumnList,
		(DWORD) (pColumns->size() * sizeof(ColumnOld_t)));

	free(pColumnList);
}

std::vector<Column_t> Explorerplusplus::LoadColumnFromRegistry(HKEY hColumnsKey,
	const TCHAR *szKeyName)
{
	ColumnOld_t columnList[64];
	Column_t column;
	DWORD dwSize;
	DWORD dwType;
	unsigned int i = 0;

	dwType = REG_BINARY;
	dwSize = sizeof(columnList);

	RegQueryValueEx(hColumnsKey, szKeyName, nullptr, &dwType, (LPBYTE) columnList, &dwSize);

	std::vector<Column_t> columns;

	for (i = 0; i < dwSize / sizeof(ColumnOld_t); i++)
	{
		column.type = static_cast<ColumnType>(columnList[i].id);
		column.bChecked = columnList[i].bChecked;
		column.iWidth = DEFAULT_COLUMN_WIDTH;

		columns.push_back(column);
	}

	return columns;
}

void Explorerplusplus::SaveDefaultColumnsToRegistry()
{
	HKEY hColumnsKey;
	DWORD disposition;
	LONG returnValue;

	/* Open/Create the main key that is used to store data. */
	returnValue = RegCreateKeyEx(HKEY_CURRENT_USER, REG_COLUMNS_KEY, 0, nullptr,
		REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hColumnsKey, &disposition);

	if (returnValue == ERROR_SUCCESS)
	{
		SaveColumnToRegistry(hColumnsKey, _T("ControlPanelColumns"),
			&m_config->globalFolderSettings.folderColumns.controlPanelColumns);
		SaveColumnWidthsToRegistry(hColumnsKey, _T("ControlPanelColumnWidths"),
			&m_config->globalFolderSettings.folderColumns.controlPanelColumns);

		SaveColumnToRegistry(hColumnsKey, _T("MyComputerColumns"),
			&m_config->globalFolderSettings.folderColumns.myComputerColumns);
		SaveColumnWidthsToRegistry(hColumnsKey, _T("MyComputerColumnWidths"),
			&m_config->globalFolderSettings.folderColumns.myComputerColumns);

		SaveColumnToRegistry(hColumnsKey, _T("RealFolderColumns"),
			&m_config->globalFolderSettings.folderColumns.realFolderColumns);
		SaveColumnWidthsToRegistry(hColumnsKey, _T("RealFolderColumnWidths"),
			&m_config->globalFolderSettings.folderColumns.realFolderColumns);

		SaveColumnToRegistry(hColumnsKey, _T("RecycleBinColumns"),
			&m_config->globalFolderSettings.folderColumns.recycleBinColumns);
		SaveColumnWidthsToRegistry(hColumnsKey, _T("RecycleBinColumnWidths"),
			&m_config->globalFolderSettings.folderColumns.recycleBinColumns);

		SaveColumnToRegistry(hColumnsKey, _T("PrinterColumns"),
			&m_config->globalFolderSettings.folderColumns.printersColumns);
		SaveColumnWidthsToRegistry(hColumnsKey, _T("PrinterColumnWidths"),
			&m_config->globalFolderSettings.folderColumns.printersColumns);

		SaveColumnToRegistry(hColumnsKey, _T("NetworkColumns"),
			&m_config->globalFolderSettings.folderColumns.networkConnectionsColumns);
		SaveColumnWidthsToRegistry(hColumnsKey, _T("NetworkColumnWidths"),
			&m_config->globalFolderSettings.folderColumns.networkConnectionsColumns);

		SaveColumnToRegistry(hColumnsKey, _T("NetworkPlacesColumns"),
			&m_config->globalFolderSettings.folderColumns.myNetworkPlacesColumns);
		SaveColumnWidthsToRegistry(hColumnsKey, _T("NetworkPlacesColumnWidths"),
			&m_config->globalFolderSettings.folderColumns.myNetworkPlacesColumns);

		RegCloseKey(hColumnsKey);
	}
}

void Explorerplusplus::LoadDefaultColumnsFromRegistry()
{
	HKEY hColumnsKey;
	LONG res;

	/* Open/Create the main key that is used to store data. */
	res = RegOpenKeyEx(HKEY_CURRENT_USER, REG_COLUMNS_KEY, 0, KEY_READ, &hColumnsKey);

	if (res == ERROR_SUCCESS)
	{
		auto &defaultFolderColumns = m_config->globalFolderSettings.folderColumns;

		defaultFolderColumns.controlPanelColumns =
			LoadColumnFromRegistry(hColumnsKey, _T("ControlPanelColumns"));
		defaultFolderColumns.myComputerColumns =
			LoadColumnFromRegistry(hColumnsKey, _T("MyComputerColumns"));
		defaultFolderColumns.realFolderColumns =
			LoadColumnFromRegistry(hColumnsKey, _T("RealFolderColumns"));
		defaultFolderColumns.recycleBinColumns =
			LoadColumnFromRegistry(hColumnsKey, _T("RecycleBinColumns"));
		defaultFolderColumns.printersColumns =
			LoadColumnFromRegistry(hColumnsKey, _T("PrinterColumns"));
		defaultFolderColumns.networkConnectionsColumns =
			LoadColumnFromRegistry(hColumnsKey, _T("NetworkColumns"));
		defaultFolderColumns.myNetworkPlacesColumns =
			LoadColumnFromRegistry(hColumnsKey, _T("NetworkPlacesColumns"));

		auto controlPanelWidths =
			LoadColumnWidthsFromRegistry(hColumnsKey, _T("ControlPanelColumnWidths"));
		auto myComputerWidths =
			LoadColumnWidthsFromRegistry(hColumnsKey, _T("MyComputerColumnWidths"));
		auto realFolderWidths =
			LoadColumnWidthsFromRegistry(hColumnsKey, _T("RealFolderColumnWidths"));
		auto recycleBinWidths =
			LoadColumnWidthsFromRegistry(hColumnsKey, _T("RecycleBinColumnWidths"));
		auto printersWidths = LoadColumnWidthsFromRegistry(hColumnsKey, _T("PrinterColumnWidths"));
		auto networkConnectionsWidths =
			LoadColumnWidthsFromRegistry(hColumnsKey, _T("NetworkColumnWidths"));
		auto myNetworkPlacesWidths =
			LoadColumnWidthsFromRegistry(hColumnsKey, _T("NetworkPlacesColumnWidths"));

		UpdateColumnWidths(defaultFolderColumns.controlPanelColumns, controlPanelWidths);
		UpdateColumnWidths(defaultFolderColumns.myComputerColumns, myComputerWidths);
		UpdateColumnWidths(defaultFolderColumns.realFolderColumns, realFolderWidths);
		UpdateColumnWidths(defaultFolderColumns.recycleBinColumns, recycleBinWidths);
		UpdateColumnWidths(defaultFolderColumns.printersColumns, printersWidths);
		UpdateColumnWidths(defaultFolderColumns.networkConnectionsColumns,
			networkConnectionsWidths);
		UpdateColumnWidths(defaultFolderColumns.myNetworkPlacesColumns, myNetworkPlacesWidths);

		ValidateColumns(defaultFolderColumns);

		RegCloseKey(hColumnsKey);
	}
}

void Explorerplusplus::SaveToolbarInformationToRegistry()
{
	HKEY hKey;
	HKEY hToolbarKey;
	REBARBANDINFO rbi;
	TCHAR szItemKey[128];
	DWORD disposition;
	LONG returnValue;
	int nBands = 0;
	int i = 0;

	/* First, delete any current rebar key. */
	SHDeleteKey(HKEY_CURRENT_USER, REG_TOOLBARS_KEY);

	returnValue = RegCreateKeyEx(HKEY_CURRENT_USER, REG_TOOLBARS_KEY, 0, nullptr,
		REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, &disposition);

	if (returnValue == ERROR_SUCCESS)
	{
		nBands = (int) SendMessage(m_hMainRebar, RB_GETBANDCOUNT, 0, 0);

		/* Use RBBIM_ID to map between windows and bands. */
		for (i = 0; i < nBands; i++)
		{
			StringCchPrintf(szItemKey, SIZEOF_ARRAY(szItemKey), _T("%d"), i);

			returnValue = RegCreateKeyEx(hKey, szItemKey, 0, nullptr, REG_OPTION_NON_VOLATILE,
				KEY_WRITE, nullptr, &hToolbarKey, &disposition);

			if (returnValue == ERROR_SUCCESS)
			{
				rbi.cbSize = sizeof(rbi);
				rbi.fMask = RBBIM_ID | RBBIM_CHILD | RBBIM_SIZE | RBBIM_STYLE;
				SendMessage(m_hMainRebar, RB_GETBANDINFO, i, (LPARAM) &rbi);

				RegistrySettings::SaveDword(hToolbarKey, _T("id"), rbi.wID);
				RegistrySettings::SaveDword(hToolbarKey, _T("Style"), rbi.fStyle);
				RegistrySettings::SaveDword(hToolbarKey, _T("Length"), rbi.cx);

				RegCloseKey(hToolbarKey);
			}
		}

		RegCloseKey(hKey);
	}
}

void Explorerplusplus::LoadToolbarInformationFromRegistry()
{
	HKEY hKey;
	HKEY hToolbarKey;
	TCHAR szItemKey[128];
	LONG deturnValue;
	int i = 0;

	deturnValue = RegOpenKeyEx(HKEY_CURRENT_USER, REG_TOOLBARS_KEY, 0, KEY_READ, &hKey);

	if (deturnValue == ERROR_SUCCESS)
	{
		StringCchPrintf(szItemKey, SIZEOF_ARRAY(szItemKey), _T("%d"), i);

		deturnValue = RegOpenKeyEx(hKey, szItemKey, 0, KEY_READ, &hToolbarKey);

		while (deturnValue == ERROR_SUCCESS)
		{
			BOOL bUseChevron = FALSE;

			if (m_ToolbarInformation[i].fStyle & RBBS_USECHEVRON)
			{
				bUseChevron = TRUE;
			}

			RegistrySettings::Read32BitValueFromRegistry(hToolbarKey, _T("id"),
				m_ToolbarInformation[i].wID);
			RegistrySettings::Read32BitValueFromRegistry(hToolbarKey, _T("Style"),
				m_ToolbarInformation[i].fStyle);
			RegistrySettings::Read32BitValueFromRegistry(hToolbarKey, _T("Length"),
				m_ToolbarInformation[i].cx);

			if (bUseChevron)
			{
				m_ToolbarInformation[i].fStyle |= RBBS_USECHEVRON;
			}

			RegCloseKey(hToolbarKey);

			i++;

			StringCchPrintf(szItemKey, SIZEOF_ARRAY(szItemKey), _T("%d"), i);

			deturnValue = RegOpenKeyEx(hKey, szItemKey, 0, KEY_READ, &hToolbarKey);
		}

		RegCloseKey(hKey);
	}
}
