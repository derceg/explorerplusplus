// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ConfigRegistryStorage.h"
#include "Config.h"
#include "CustomFontStorage.h"
#include "StartupFoldersRegistryStorage.h"
#include "Storage.h"
#include "../Helper/RegistrySettings.h"
#include <wil/registry.h>

namespace
{

constexpr wchar_t MAIN_FONT_KEY_NAME[] = L"MainFont";
constexpr wchar_t STARTUP_FOLDERS_KEY_NAME[] = L"StartupFolders";

void LoadFromKey(HKEY settingsKey, Config &config)
{
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ShowExtensions",
		config.globalFolderSettings.showExtensions);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ShowStatusBar",
		config.showStatusBar);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ShowFolders", config.showFolders);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ShowAddressBar",
		config.showAddressBar);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ShowToolbar",
		config.showMainToolbar);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ShowBookmarksToolbar",
		config.showBookmarksToolbar);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ShowDrivesToolbar",
		config.showDrivesToolbar);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ShowApplicationToolbar",
		config.showApplicationToolbar);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"AlwaysOpenNewTab",
		config.alwaysOpenNewTab);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ShowFriendlyDates",
		config.globalFolderSettings.showFriendlyDates);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ShowDisplayWindow",
		config.showDisplayWindow);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ShowFolderSizes",
		config.globalFolderSettings.showFolderSizes);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"DisableFolderSizesNetworkRemovable",
		config.globalFolderSettings.disableFolderSizesNetworkRemovable);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"NextToCurrent",
		config.openNewTabNextToCurrent);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ConfirmCloseTabs",
		config.confirmCloseTabs);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ShowInfoTips", config.showInfoTips);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"TreeViewDelayEnabled",
		config.treeViewDelayEnabled);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"LockToolbars", config.lockToolbars);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"UseFullRowSelect",
		config.useFullRowSelect);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ShowFilePreviews",
		config.showFilePreviews);
	RegistrySettings::ReadBetterEnumValue(settingsKey, L"ReplaceExplorerMode",
		config.replaceExplorerMode);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ShowFullTitlePath",
		config.showFullTitlePath);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ShowUserNameTitleBar",
		config.showUserNameInTitleBar);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ShowPrivilegeLevelInTitleBar",
		config.showPrivilegeLevelInTitleBar);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"AlwaysShowTabBar",
		config.alwaysShowTabBar);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ShowTabBarAtBottom",
		config.showTabBarAtBottom);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ExtendTabControl",
		config.extendTabControl);
	RegistrySettings::ReadBetterEnumValue(settingsKey, L"StartupMode", config.startupMode);
	RegistrySettings::ReadBetterEnumValue(settingsKey, L"InfoTipType", config.infoTipType);
	RegistrySettings::ReadBetterEnumValue(settingsKey, L"SizeDisplayFormat",
		config.globalFolderSettings.sizeDisplayFormat);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"AllowMultipleInstances",
		config.allowMultipleInstances);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"OneClickActivate",
		config.globalFolderSettings.oneClickActivate);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"OneClickActivateHoverTime",
		config.globalFolderSettings.oneClickActivateHoverTime);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"DoubleClickTabClose",
		config.doubleClickTabClose);

	auto res = RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"OpenContainerFiles",
		config.openContainerFiles);

	if (res != ERROR_SUCCESS)
	{
		// Previously, there was a single option used to indicate whether zip files should be opened
		// in Explorer++.
		RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"HandleZipFiles",
			config.openContainerFiles);
	}

	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"InsertSorted",
		config.globalFolderSettings.insertSorted);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"CheckBoxSelection",
		config.checkBoxSelection);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ForceSize",
		config.globalFolderSettings.forceSize);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"CloseMainWindowOnTabClose",
		config.closeMainWindowOnTabClose);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ShowTaskbarThumbnails",
		config.showTaskbarThumbnails);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"SynchronizeTreeview",
		config.synchronizeTreeview);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"TVAutoExpandSelected",
		config.treeViewAutoExpandSelected);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"OverwriteExistingFilesConfirmation",
		config.overwriteExistingFilesConfirmation);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"LargeToolbarIcons",
		config.useLargeToolbarIcons);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"CheckPinnedToNamespaceTreeProperty",
		config.checkPinnedToNamespaceTreeProperty);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ShowQuickAccessInTreeView",
		config.showQuickAccessInTreeView);

	auto theme = config.theme.get();
	res = RegistrySettings::ReadBetterEnumValue(settingsKey, L"Theme", theme);

	if (res != ERROR_SUCCESS)
	{
		RegistrySettings::ReadDword(settingsKey, L"EnableDarkMode",
			[&theme](DWORD value) { theme = value ? Theme::Dark : Theme::Light; });
	}

	config.theme = theme;

	RegistrySettings::ReadString(settingsKey, L"NewTabDirectory", config.defaultTabDirectory);
	RegistrySettings::ReadBetterEnumValue(settingsKey, L"IconTheme", config.iconSet);

	DWORD language;
	res = RegistrySettings::ReadDword(settingsKey, L"Language", language);

	if (res == ERROR_SUCCESS)
	{
		config.language = static_cast<LANGID>(language);
	}

	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"OpenTabsInForeground",
		config.openTabsInForeground);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"DisplayMixedFilesAndFolders",
		config.globalFolderSettings.displayMixedFilesAndFolders);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"UseNaturalSortOrder",
		config.globalFolderSettings.useNaturalSortOrder);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"GoUpOnDoubleClick",
		config.goUpOnDoubleClick);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ShowHiddenGlobal",
		config.defaultFolderSettings.showHidden);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ShowGridlinesGlobal",
		config.globalFolderSettings.showGridlines);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"ShowInGroupsGlobal",
		config.defaultFolderSettings.showInGroups);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"AutoArrangeGlobal",
		config.defaultFolderSettings.autoArrange);
	RegistrySettings::ReadDword(settingsKey, L"SortAscendingGlobal",
		[&config](DWORD value)
		{
			config.defaultFolderSettings.sortDirection =
				value ? SortDirection::Ascending : SortDirection::Descending;
			config.defaultFolderSettings.groupSortDirection =
				value ? SortDirection::Ascending : SortDirection::Descending;
		});
	RegistrySettings::ReadBetterEnumValue(settingsKey, L"GroupSortDirectionGlobal",
		config.defaultFolderSettings.groupSortDirection);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"HideSystemFilesGlobal",
		config.globalFolderSettings.hideSystemFiles);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"HideLinkExtensionGlobal",
		config.globalFolderSettings.hideLinkExtension);
	RegistrySettings::ReadBetterEnumValue(settingsKey, L"ViewModeGlobal",
		config.defaultFolderSettings.viewMode);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"DisplayWindowVertical",
		config.displayWindowVertical);

	COLORREF surroundColor;
	res = RegistrySettings::ReadBinaryValue(settingsKey, L"DisplaySurroundColor", &surroundColor,
		sizeof(surroundColor));

	if (res == ERROR_SUCCESS)
	{
		config.displayWindowSurroundColor = surroundColor;
	}

	COLORREF centreColor;
	res = RegistrySettings::ReadBinaryValue(settingsKey, L"DisplayCentreColor", &centreColor,
		sizeof(centreColor));

	if (res == ERROR_SUCCESS)
	{
		config.displayWindowCentreColor = centreColor;
	}

	COLORREF textColor;
	res = RegistrySettings::ReadBinaryValue(settingsKey, L"DisplayTextColor", &textColor,
		sizeof(textColor));

	if (res == ERROR_SUCCESS)
	{
		config.displayWindowTextColor = textColor;
	}

	LOGFONT font;
	res = RegistrySettings::ReadBinaryValue(settingsKey, L"DisplayFont", &font, sizeof(font));

	if (res == ERROR_SUCCESS)
	{
		config.displayWindowFont = font;
	}

	wil::unique_hkey mainFontKey;
	HRESULT hr = wil::reg::open_unique_key_nothrow(settingsKey, MAIN_FONT_KEY_NAME, mainFontKey,
		wil::reg::key_access::read);

	if (SUCCEEDED(hr))
	{
		auto mainFont = CustomFontStorage::LoadFromRegistry(mainFontKey.get());

		if (mainFont)
		{
			config.mainFont = *mainFont;
		}
	}

	wil::unique_hkey startupFoldersKey;
	hr = wil::reg::open_unique_key_nothrow(settingsKey, STARTUP_FOLDERS_KEY_NAME, startupFoldersKey,
		wil::reg::key_access::read);

	if (SUCCEEDED(hr))
	{
		config.startupFolders = StartupFoldersRegistryStorage::Load(startupFoldersKey.get());
	}
}

void SaveToKey(HKEY settingsKey, const Config &config)
{
	RegistrySettings::SaveDword(settingsKey, L"ShowExtensions",
		config.globalFolderSettings.showExtensions);
	RegistrySettings::SaveDword(settingsKey, L"ShowStatusBar", config.showStatusBar);
	RegistrySettings::SaveDword(settingsKey, L"ShowFolders", config.showFolders.get());
	RegistrySettings::SaveDword(settingsKey, L"ShowAddressBar", config.showAddressBar.get());
	RegistrySettings::SaveDword(settingsKey, L"ShowToolbar", config.showMainToolbar.get());
	RegistrySettings::SaveDword(settingsKey, L"ShowBookmarksToolbar",
		config.showBookmarksToolbar.get());
	RegistrySettings::SaveDword(settingsKey, L"ShowDrivesToolbar", config.showDrivesToolbar.get());
	RegistrySettings::SaveDword(settingsKey, L"ShowApplicationToolbar",
		config.showApplicationToolbar.get());
	RegistrySettings::SaveDword(settingsKey, L"ShowFullTitlePath", config.showFullTitlePath.get());
	RegistrySettings::SaveDword(settingsKey, L"AlwaysOpenNewTab", config.alwaysOpenNewTab);
	RegistrySettings::SaveDword(settingsKey, L"ShowFriendlyDates",
		config.globalFolderSettings.showFriendlyDates);
	RegistrySettings::SaveDword(settingsKey, L"ShowDisplayWindow", config.showDisplayWindow.get());
	RegistrySettings::SaveDword(settingsKey, L"ShowFolderSizes",
		config.globalFolderSettings.showFolderSizes);
	RegistrySettings::SaveDword(settingsKey, L"DisableFolderSizesNetworkRemovable",
		config.globalFolderSettings.disableFolderSizesNetworkRemovable);
	RegistrySettings::SaveDword(settingsKey, L"StartupMode", config.startupMode);
	RegistrySettings::SaveDword(settingsKey, L"NextToCurrent", config.openNewTabNextToCurrent);
	RegistrySettings::SaveDword(settingsKey, L"ConfirmCloseTabs", config.confirmCloseTabs);
	RegistrySettings::SaveDword(settingsKey, L"ShowInfoTips", config.showInfoTips);
	RegistrySettings::SaveDword(settingsKey, L"InfoTipType", config.infoTipType);
	RegistrySettings::SaveDword(settingsKey, L"TreeViewDelayEnabled", config.treeViewDelayEnabled);
	RegistrySettings::SaveDword(settingsKey, L"LockToolbars", config.lockToolbars.get());
	RegistrySettings::SaveDword(settingsKey, L"ExtendTabControl", config.extendTabControl.get());
	RegistrySettings::SaveDword(settingsKey, L"UseFullRowSelect", config.useFullRowSelect.get());
	RegistrySettings::SaveDword(settingsKey, L"ShowFilePreviews", config.showFilePreviews);
	RegistrySettings::SaveDword(settingsKey, L"ReplaceExplorerMode", config.replaceExplorerMode);
	RegistrySettings::SaveDword(settingsKey, L"ShowUserNameTitleBar",
		config.showUserNameInTitleBar.get());
	RegistrySettings::SaveDword(settingsKey, L"AllowMultipleInstances",
		config.allowMultipleInstances);
	RegistrySettings::SaveDword(settingsKey, L"OneClickActivate",
		config.globalFolderSettings.oneClickActivate.get());
	RegistrySettings::SaveDword(settingsKey, L"OneClickActivateHoverTime",
		config.globalFolderSettings.oneClickActivateHoverTime.get());
	RegistrySettings::SaveDword(settingsKey, L"DoubleClickTabClose", config.doubleClickTabClose);
	RegistrySettings::SaveDword(settingsKey, L"OpenContainerFiles", config.openContainerFiles);
	RegistrySettings::SaveDword(settingsKey, L"InsertSorted",
		config.globalFolderSettings.insertSorted);
	RegistrySettings::SaveDword(settingsKey, L"ShowPrivilegeLevelInTitleBar",
		config.showPrivilegeLevelInTitleBar.get());
	RegistrySettings::SaveDword(settingsKey, L"AlwaysShowTabBar", config.alwaysShowTabBar.get());
	RegistrySettings::SaveDword(settingsKey, L"CheckBoxSelection", config.checkBoxSelection.get());
	RegistrySettings::SaveDword(settingsKey, L"ForceSize", config.globalFolderSettings.forceSize);
	RegistrySettings::SaveDword(settingsKey, L"SizeDisplayFormat",
		config.globalFolderSettings.sizeDisplayFormat);
	RegistrySettings::SaveDword(settingsKey, L"CloseMainWindowOnTabClose",
		config.closeMainWindowOnTabClose);
	RegistrySettings::SaveDword(settingsKey, L"ShowTabBarAtBottom",
		config.showTabBarAtBottom.get());
	RegistrySettings::SaveDword(settingsKey, L"OverwriteExistingFilesConfirmation",
		config.overwriteExistingFilesConfirmation);
	RegistrySettings::SaveDword(settingsKey, L"LargeToolbarIcons",
		config.useLargeToolbarIcons.get());
	RegistrySettings::SaveDword(settingsKey, L"CheckPinnedToNamespaceTreeProperty",
		config.checkPinnedToNamespaceTreeProperty);
	RegistrySettings::SaveDword(settingsKey, L"ShowQuickAccessInTreeView",
		config.showQuickAccessInTreeView.get());
	RegistrySettings::SaveDword(settingsKey, L"Theme", config.theme.get());
	RegistrySettings::SaveString(settingsKey, L"NewTabDirectory", config.defaultTabDirectory);
	RegistrySettings::SaveDword(settingsKey, L"IconTheme", config.iconSet);
	RegistrySettings::SaveDword(settingsKey, L"Language", config.language);
	RegistrySettings::SaveDword(settingsKey, L"OpenTabsInForeground", config.openTabsInForeground);
	RegistrySettings::SaveDword(settingsKey, L"DisplayMixedFilesAndFolders",
		config.globalFolderSettings.displayMixedFilesAndFolders);
	RegistrySettings::SaveDword(settingsKey, L"UseNaturalSortOrder",
		config.globalFolderSettings.useNaturalSortOrder);
	RegistrySettings::SaveDword(settingsKey, L"GoUpOnDoubleClick", config.goUpOnDoubleClick);
	RegistrySettings::SaveDword(settingsKey, L"ShowHiddenGlobal",
		config.defaultFolderSettings.showHidden);
	RegistrySettings::SaveDword(settingsKey, L"ViewModeGlobal",
		config.defaultFolderSettings.viewMode);
	RegistrySettings::SaveDword(settingsKey, L"ShowGridlinesGlobal",
		config.globalFolderSettings.showGridlines.get());
	RegistrySettings::SaveDword(settingsKey, L"ShowInGroupsGlobal",
		config.defaultFolderSettings.showInGroups);
	RegistrySettings::SaveDword(settingsKey, L"AutoArrangeGlobal",
		config.defaultFolderSettings.autoArrange);
	RegistrySettings::SaveDword(settingsKey, L"SortAscendingGlobal",
		config.defaultFolderSettings.sortDirection == +SortDirection::Ascending);
	RegistrySettings::SaveDword(settingsKey, L"GroupSortDirectionGlobal",
		config.defaultFolderSettings.groupSortDirection);
	RegistrySettings::SaveDword(settingsKey, L"HideSystemFilesGlobal",
		config.globalFolderSettings.hideSystemFiles);
	RegistrySettings::SaveDword(settingsKey, L"HideLinkExtensionGlobal",
		config.globalFolderSettings.hideLinkExtension);
	RegistrySettings::SaveDword(settingsKey, L"ShowTaskbarThumbnails",
		config.showTaskbarThumbnails);
	RegistrySettings::SaveDword(settingsKey, L"SynchronizeTreeview",
		config.synchronizeTreeview.get());
	RegistrySettings::SaveDword(settingsKey, L"TVAutoExpandSelected",
		config.treeViewAutoExpandSelected);

	RegistrySettings::SaveDword(settingsKey, L"DisplayWindowVertical",
		config.displayWindowVertical);

	RegistrySettings::SaveBinaryValue(settingsKey, L"DisplayCentreColor",
		reinterpret_cast<const BYTE *>(&config.displayWindowCentreColor.get()),
		sizeof(config.displayWindowCentreColor.get()));
	RegistrySettings::SaveBinaryValue(settingsKey, L"DisplaySurroundColor",
		reinterpret_cast<const BYTE *>(&config.displayWindowSurroundColor.get()),
		sizeof(config.displayWindowSurroundColor.get()));
	RegistrySettings::SaveBinaryValue(settingsKey, L"DisplayTextColor",
		reinterpret_cast<const BYTE *>(&config.displayWindowTextColor.get()),
		sizeof(config.displayWindowTextColor.get()));
	RegistrySettings::SaveBinaryValue(settingsKey, L"DisplayFont",
		reinterpret_cast<const BYTE *>(&config.displayWindowFont.get()),
		sizeof(config.displayWindowFont.get()));

	auto &mainFont = config.mainFont.get();

	if (mainFont)
	{
		wil::unique_hkey mainFontKey;
		HRESULT hr = wil::reg::create_unique_key_nothrow(settingsKey, MAIN_FONT_KEY_NAME,
			mainFontKey, wil::reg::key_access::readwrite);

		if (SUCCEEDED(hr))
		{
			CustomFontStorage::SaveToRegistry(mainFontKey.get(), *mainFont);
		}
	}

	wil::unique_hkey startupFoldersKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(settingsKey, STARTUP_FOLDERS_KEY_NAME,
		startupFoldersKey, wil::reg::key_access::readwrite);

	if (SUCCEEDED(hr))
	{
		StartupFoldersRegistryStorage::Save(startupFoldersKey.get(), config.startupFolders);
	}
}

}

namespace ConfigRegistryStorage
{

void Load(HKEY applicationKey, Config &config)
{
	wil::unique_hkey settingsKey;
	HRESULT hr = wil::reg::open_unique_key_nothrow(applicationKey,
		Storage::REGISTRY_SETTINGS_KEY_NAME, settingsKey, wil::reg::key_access::read);

	if (FAILED(hr))
	{
		return;
	}

	LoadFromKey(settingsKey.get(), config);
}

void Save(HKEY applicationKey, const Config &config)
{
	wil::unique_hkey settingsKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(applicationKey,
		Storage::REGISTRY_SETTINGS_KEY_NAME, settingsKey, wil::reg::key_access::readwrite);

	if (FAILED(hr))
	{
		return;
	}

	SaveToKey(settingsKey.get(), config);
}

}
