// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "ColumnRegistryStorage.h"
#include "Config.h"
#include "CustomFontStorage.h"
#include "DefaultColumns.h"
#include "DisplayWindow/DisplayWindow.h"
#include "Explorer++_internal.h"
#include "MainRebarRegistryStorage.h"
#include "MainRebarStorage.h"
#include "MainToolbar.h"
#include "ShellBrowser/ShellBrowser.h"
#include "TabContainer.h"
#include "TabRegistryStorage.h"
#include "TabStorage.h"
#include "../Helper/Macros.h"
#include "../Helper/RegistrySettings.h"
#include <wil/registry.h>

using namespace std::string_literals;

namespace
{
const TCHAR REG_TABS_KEY[] = _T("Software\\Explorer++\\Tabs");
const TCHAR REG_DEFAULT_COLUMNS_KEY[] = _T("Software\\Explorer++\\DefaultColumns");
const TCHAR REG_MAIN_FONT_KEY_NAME[] = _T("MainFont");
}

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

		SHDeleteKey(hSettingsKey, REG_MAIN_FONT_KEY_NAME);

		auto &mainFont = m_config->mainFont.get();

		if (mainFont)
		{
			auto mainFontKeyPath =
				NExplorerplusplus::REG_SETTINGS_KEY + L"\\"s + REG_MAIN_FONT_KEY_NAME;
			SaveCustomFontToRegistry(mainFontKeyPath, *mainFont);
		}

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

		auto mainFont = LoadCustomFontFromRegistry(
			NExplorerplusplus::REG_SETTINGS_KEY + L"\\"s + REG_MAIN_FONT_KEY_NAME);

		if (mainFont)
		{
			m_config->mainFont = *mainFont;
		}

		RegCloseKey(hSettingsKey);
	}

	return returnValue;
}

void Explorerplusplus::SaveTabSettingsToRegistry()
{
	SHDeleteKey(HKEY_CURRENT_USER, REG_TABS_KEY);

	wil::unique_hkey tabsKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(HKEY_CURRENT_USER, REG_TABS_KEY, tabsKey,
		wil::reg::key_access::readwrite);

	if (FAILED(hr))
	{
		return;
	}

	TabRegistryStorage::Save(tabsKey.get(), GetTabListStorageData());
}

void Explorerplusplus::LoadTabSettingsFromRegistry()
{
	wil::unique_hkey tabsKey;
	HRESULT hr = wil::reg::open_unique_key_nothrow(HKEY_CURRENT_USER, REG_TABS_KEY, tabsKey,
		wil::reg::key_access::read);

	if (FAILED(hr))
	{
		return;
	}

	m_loadedTabs = TabRegistryStorage::Load(tabsKey.get());
}

void Explorerplusplus::SaveDefaultColumnsToRegistry()
{
	wil::unique_hkey defaultColumnsKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(HKEY_CURRENT_USER, REG_DEFAULT_COLUMNS_KEY,
		defaultColumnsKey, wil::reg::key_access::readwrite);

	if (FAILED(hr))
	{
		return;
	}

	ColumnRegistryStorage::SaveAllColumnSets(defaultColumnsKey.get(),
		m_config->globalFolderSettings.folderColumns);
}

void Explorerplusplus::LoadDefaultColumnsFromRegistry()
{
	wil::unique_hkey defaultColumnsKey;
	HRESULT hr = wil::reg::open_unique_key_nothrow(HKEY_CURRENT_USER, REG_DEFAULT_COLUMNS_KEY,
		defaultColumnsKey, wil::reg::key_access::read);

	if (FAILED(hr))
	{
		return;
	}

	auto &defaultFolderColumns = m_config->globalFolderSettings.folderColumns;
	ColumnRegistryStorage::LoadAllColumnSets(defaultColumnsKey.get(), defaultFolderColumns);
}

void Explorerplusplus::LoadMainRebarInformationFromRegistry(HKEY mainKey)
{
	m_loadedRebarStorageInfo = MainRebarRegistryStorage::Load(mainKey);
}

void Explorerplusplus::SaveMainRebarInformationToRegistry(HKEY mainKey)
{
	MainRebarRegistryStorage::Save(mainKey, GetMainRebarStorageInfo());
}
