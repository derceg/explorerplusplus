// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ConfigXmlStorage.h"
#include "Config.h"
#include "CustomFontStorage.h"
#include "Storage.h"
#include "../Helper/XMLSettings.h"
#include <wil/com.h>
#include <format>

namespace
{

template <typename T>
concept IntegralSetting = std::is_integral_v<T> && !std::is_same_v<T, bool>;

template <class T>
concept WrappedIntegralSetting = requires(T t) {
	[]<IntegralSetting U>(ValueWrapper<U> &) {
	}(t);
};

HRESULT GetSettingNode(IXMLDOMNode *settingsNode, const std::wstring &settingName,
	IXMLDOMNode **outputNode)
{
	auto query = wil::make_bstr_nothrow(std::format(L"Setting[@name='{}']", settingName).c_str());
	return settingsNode->selectSingleNode(query.get(), outputNode);
}

HRESULT GetTextSetting(IXMLDOMNode *settingsNode, const std::wstring &settingName,
	std::wstring &outputText)
{
	wil::com_ptr_nothrow<IXMLDOMNode> settingNode;
	HRESULT hr = GetSettingNode(settingsNode, settingName, &settingNode);

	if (hr != S_OK)
	{
		return hr;
	}

	wil::unique_bstr text;
	hr = settingNode->get_text(&text);

	if (hr != S_OK)
	{
		return hr;
	}

	outputText = wil::str_raw_ptr(text);

	return hr;
}

template <typename T>
	requires IntegralSetting<T> || WrappedIntegralSetting<T>
HRESULT GetIntSetting(IXMLDOMNode *settingsNode, const std::wstring &settingName, T &outputValue)
{
	std::wstring text;
	HRESULT hr = GetTextSetting(settingsNode, settingName, text);

	if (hr != S_OK)
	{
		return hr;
	}

	outputValue = XMLSettings::DecodeIntValue(text.c_str());

	return hr;
}

template <typename T>
	requires std::same_as<T, bool> || std::same_as<T, ValueWrapper<bool>>
HRESULT GetBoolSetting(IXMLDOMNode *settingsNode, const std::wstring &settingName, T &outputValue)
{
	std::wstring text;
	HRESULT hr = GetTextSetting(settingsNode, settingName, text);

	if (hr != S_OK)
	{
		return hr;
	}

	outputValue = XMLSettings::DecodeBoolValue(text.c_str());

	return hr;
}

template <BetterEnum T>
HRESULT GetBetterEnumSetting(IXMLDOMNode *settingsNode, const std::wstring &settingName,
	T &outputValue)
{
	int value;
	HRESULT hr = GetIntSetting(settingsNode, settingName, value);

	if (hr != S_OK)
	{
		return hr;
	}

	if (!T::_is_valid(value))
	{
		return E_FAIL;
	}

	outputValue = T::_from_integral(value);

	return hr;
}

void LoadFromNode(IXMLDOMNode *settingsNode, Config &config)
{
	GetBoolSetting(settingsNode, L"AllowMultipleInstances", config.allowMultipleInstances);
	GetBoolSetting(settingsNode, L"AlwaysOpenInNewTab", config.alwaysOpenNewTab);
	GetBoolSetting(settingsNode, L"AlwaysShowTabBar", config.alwaysShowTabBar);
	GetBoolSetting(settingsNode, L"AutoArrangeGlobal", config.defaultFolderSettings.autoArrange);
	GetBoolSetting(settingsNode, L"CheckBoxSelection", config.checkBoxSelection);
	GetBoolSetting(settingsNode, L"CloseMainWindowOnTabClose", config.closeMainWindowOnTabClose);
	GetBoolSetting(settingsNode, L"ConfirmCloseTabs", config.confirmCloseTabs);
	GetBoolSetting(settingsNode, L"DisableFolderSizesNetworkRemovable",
		config.globalFolderSettings.disableFolderSizesNetworkRemovable);

	if (wil::com_ptr_nothrow<IXMLDOMNode> node;
		GetSettingNode(settingsNode, L"DisplayCentreColor", &node) == S_OK)
	{
		config.displayWindowCentreColor = XMLSettings::ReadXMLColorData(node.get());
	}

	if (wil::com_ptr_nothrow<IXMLDOMNode> node;
		GetSettingNode(settingsNode, L"DisplayFont", &node) == S_OK)
	{
		config.displayWindowFont = XMLSettings::ReadXMLFontData(node.get());
	}

	if (wil::com_ptr_nothrow<IXMLDOMNode> node;
		GetSettingNode(settingsNode, L"DisplaySurroundColor", &node) == S_OK)
	{
		config.displayWindowSurroundColor = XMLSettings::ReadXMLColorData(node.get());
	}

	if (wil::com_ptr_nothrow<IXMLDOMNode> node;
		GetSettingNode(settingsNode, L"DisplayTextColor", &node) == S_OK)
	{
		config.displayWindowTextColor = XMLSettings::ReadXMLColorData(node.get());
	}

	GetIntSetting(settingsNode, L"DisplayWindowWidth", config.displayWindowWidth);
	GetIntSetting(settingsNode, L"DisplayWindowHeight", config.displayWindowHeight);
	GetBoolSetting(settingsNode, L"DisplayWindowVertical", config.displayWindowVertical);
	GetBoolSetting(settingsNode, L"DoubleClickTabClose", config.doubleClickTabClose);
	GetBoolSetting(settingsNode, L"ExtendTabControl", config.extendTabControl);
	GetBoolSetting(settingsNode, L"ForceSize", config.globalFolderSettings.forceSize);
	GetBoolSetting(settingsNode, L"HandleZipFiles", config.handleZipFiles);
	GetBoolSetting(settingsNode, L"HideLinkExtensionGlobal",
		config.globalFolderSettings.hideLinkExtension);
	GetBoolSetting(settingsNode, L"HideSystemFilesGlobal",
		config.globalFolderSettings.hideSystemFiles);
	GetBoolSetting(settingsNode, L"InsertSorted", config.globalFolderSettings.insertSorted);
	GetIntSetting(settingsNode, L"Language", config.language);
	GetBoolSetting(settingsNode, L"LargeToolbarIcons", config.useLargeToolbarIcons);
	GetBoolSetting(settingsNode, L"LockToolbars", config.lockToolbars);
	GetBoolSetting(settingsNode, L"NextToCurrent", config.openNewTabNextToCurrent);
	GetBoolSetting(settingsNode, L"OneClickActivate", config.globalFolderSettings.oneClickActivate);
	GetIntSetting(settingsNode, L"OneClickActivateHoverTime",
		config.globalFolderSettings.oneClickActivateHoverTime);
	GetBoolSetting(settingsNode, L"OverwriteExistingFilesConfirmation",
		config.overwriteExistingFilesConfirmation);
	GetBetterEnumSetting(settingsNode, L"ReplaceExplorerMode", config.replaceExplorerMode);
	GetBoolSetting(settingsNode, L"ShowAddressBar", config.showAddressBar);
	GetBoolSetting(settingsNode, L"ShowApplicationToolbar", config.showApplicationToolbar);
	GetBoolSetting(settingsNode, L"ShowBookmarksToolbar", config.showBookmarksToolbar);
	GetBoolSetting(settingsNode, L"ShowDrivesToolbar", config.showDrivesToolbar);
	GetBoolSetting(settingsNode, L"ShowDisplayWindow", config.showDisplayWindow);
	GetBoolSetting(settingsNode, L"ShowExtensions", config.globalFolderSettings.showExtensions);
	GetBoolSetting(settingsNode, L"ShowFilePreviews", config.showFilePreviews);
	GetBoolSetting(settingsNode, L"ShowFolders", config.showFolders);
	GetBoolSetting(settingsNode, L"ShowFolderSizes", config.globalFolderSettings.showFolderSizes);
	GetBoolSetting(settingsNode, L"ShowFriendlyDates",
		config.globalFolderSettings.showFriendlyDates);
	GetBoolSetting(settingsNode, L"ShowFullTitlePath", config.showFullTitlePath);
	GetBoolSetting(settingsNode, L"ShowGridlinesGlobal", config.globalFolderSettings.showGridlines);
	GetBoolSetting(settingsNode, L"ShowHiddenGlobal", config.defaultFolderSettings.showHidden);
	GetBoolSetting(settingsNode, L"ShowInfoTips", config.showInfoTips);
	GetBoolSetting(settingsNode, L"ShowInGroupsGlobal", config.defaultFolderSettings.showInGroups);
	GetBoolSetting(settingsNode, L"ShowPrivilegeLevelInTitleBar",
		config.showPrivilegeLevelInTitleBar);
	GetBoolSetting(settingsNode, L"ShowStatusBar", config.showStatusBar);
	GetBoolSetting(settingsNode, L"ShowTabBarAtBottom", config.showTabBarAtBottom);
	GetBoolSetting(settingsNode, L"ShowTaskbarThumbnails", config.showTaskbarThumbnails);
	GetBoolSetting(settingsNode, L"ShowToolbar", config.showMainToolbar);
	GetBoolSetting(settingsNode, L"ShowUserNameTitleBar", config.showUserNameInTitleBar);
	GetBetterEnumSetting(settingsNode, L"SizeDisplayFormat",
		config.globalFolderSettings.sizeDisplayFormat);
	GetBetterEnumSetting(settingsNode, L"StartupMode", config.startupMode);
	GetBoolSetting(settingsNode, L"SynchronizeTreeview", config.synchronizeTreeview);
	GetBoolSetting(settingsNode, L"TVAutoExpandSelected", config.treeViewAutoExpandSelected);
	GetBoolSetting(settingsNode, L"UseFullRowSelect", config.useFullRowSelect);
	GetBoolSetting(settingsNode, L"TreeViewDelayEnabled", config.treeViewDelayEnabled);
	GetIntSetting(settingsNode, L"TreeViewWidth", config.treeViewWidth);
	GetBetterEnumSetting(settingsNode, L"ViewModeGlobal", config.defaultFolderSettings.viewMode);
	GetTextSetting(settingsNode, L"NewTabDirectory", config.defaultTabDirectory);
	GetBetterEnumSetting(settingsNode, L"InfoTipType", config.infoTipType);
	GetBetterEnumSetting(settingsNode, L"IconTheme", config.iconSet);
	GetBoolSetting(settingsNode, L"CheckPinnedToNamespaceTreeProperty",
		config.checkPinnedToNamespaceTreeProperty);
	GetBoolSetting(settingsNode, L"ShowQuickAccessInTreeView", config.showQuickAccessInTreeView);

	auto theme = config.theme.get();

	if (GetBetterEnumSetting(settingsNode, L"Theme", theme) != S_OK)
	{
		if (bool enableDarkMode;
			GetBoolSetting(settingsNode, L"EnableDarkMode", enableDarkMode) == S_OK)
		{
			theme = enableDarkMode ? Theme::Dark : Theme::Light;
		}
	}

	config.theme = theme;

	GetBoolSetting(settingsNode, L"DisplayMixedFilesAndFolders",
		config.globalFolderSettings.displayMixedFilesAndFolders);
	GetBoolSetting(settingsNode, L"UseNaturalSortOrder",
		config.globalFolderSettings.useNaturalSortOrder);
	GetBoolSetting(settingsNode, L"OpenTabsInForeground", config.openTabsInForeground);

	if (bool sortAscending;
		GetBoolSetting(settingsNode, L"SortAscendingGlobal", sortAscending) == S_OK)
	{
		config.defaultFolderSettings.sortDirection =
			sortAscending ? SortDirection::Ascending : SortDirection::Descending;
		config.defaultFolderSettings.groupSortDirection =
			sortAscending ? SortDirection::Ascending : SortDirection::Descending;
	}

	GetBetterEnumSetting(settingsNode, L"GroupSortDirectionGlobal",
		config.defaultFolderSettings.groupSortDirection);
	GetBoolSetting(settingsNode, L"GoUpOnDoubleClick", config.goUpOnDoubleClick);

	if (wil::com_ptr_nothrow<IXMLDOMNode> node;
		GetSettingNode(settingsNode, L"MainFont", &node) == S_OK)
	{
		auto mainFont = CustomFontStorage::LoadFromXml(node.get());

		if (mainFont)
		{
			config.mainFont = *mainFont;
		}
	}
}

}

namespace ConfigXmlStorage
{

void Load(IXMLDOMNode *rootNode, Config &config)
{
	wil::com_ptr_nothrow<IXMLDOMNode> settingsNode;
	auto queryString = wil::make_bstr_nothrow(Storage::CONFIG_FILE_SETTINGS_NODE_NAME);
	HRESULT hr = rootNode->selectSingleNode(queryString.get(), &settingsNode);

	if (hr != S_OK)
	{
		return;
	}

	LoadFromNode(settingsNode.get(), config);
}

}
