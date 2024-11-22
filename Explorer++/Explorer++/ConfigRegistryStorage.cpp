// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ConfigRegistryStorage.h"
#include "Config.h"
#include "CustomFontStorage.h"
#include "Storage.h"
#include "../Helper/RegistrySettings.h"
#include <wil/registry.h>

namespace
{

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
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"TreeViewWidth",
		config.treeViewWidth);
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
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"HandleZipFiles",
		config.handleZipFiles);
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
	auto res = RegistrySettings::ReadBetterEnumValue(settingsKey, L"Theme", theme);

	if (res != ERROR_SUCCESS)
	{
		RegistrySettings::ReadDword(settingsKey, L"EnableDarkMode",
			[&theme](DWORD value) { theme = value ? Theme::Dark : Theme::Light; });
	}

	config.theme = theme;

	RegistrySettings::ReadString(settingsKey, L"NewTabDirectory", config.defaultTabDirectory);
	RegistrySettings::ReadBetterEnumValue(settingsKey, L"IconTheme", config.iconSet);
	RegistrySettings::ReadDword(settingsKey, L"Language", config.language);
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
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"DisplayWindowWidth",
		config.displayWindowWidth);
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, L"DisplayWindowHeight",
		config.displayWindowHeight);
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
	HRESULT hr = wil::reg::open_unique_key_nothrow(settingsKey,
		ConfigRegistryStorage::MAIN_FONT_KEY_NAME, mainFontKey, wil::reg::key_access::read);

	if (SUCCEEDED(hr))
	{
		auto mainFont = CustomFontStorage::LoadFromRegistry(mainFontKey.get());

		if (mainFont)
		{
			config.mainFont = *mainFont;
		}
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

}
