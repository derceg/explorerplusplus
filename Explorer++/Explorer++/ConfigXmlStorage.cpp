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

constexpr wchar_t SETTING_NODE_NAME[] = L"Setting";

HRESULT GetSettingNode(IXMLDOMNode *settingsNode, const std::wstring &settingName,
	IXMLDOMNode **outputNode)
{
	auto query = wil::make_bstr_nothrow(
		std::format(L"{}[@name='{}']", SETTING_NODE_NAME, settingName).c_str());
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

	DWORD language;
	HRESULT hr = GetIntSetting(settingsNode, L"Language", language);

	if (hr == S_OK)
	{
		config.language = static_cast<LANGID>(language);
	}

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

void SaveToNode(IXMLDOMDocument *xmlDocument, IXMLDOMElement *settingsNode, const Config &config)
{
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"AllowMultipleInstances", XMLSettings::EncodeBoolValue(config.allowMultipleInstances));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"AlwaysOpenInNewTab", XMLSettings::EncodeBoolValue(config.alwaysOpenNewTab));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"AlwaysShowTabBar", XMLSettings::EncodeBoolValue(config.alwaysShowTabBar.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"AutoArrangeGlobal",
		XMLSettings::EncodeBoolValue(config.defaultFolderSettings.autoArrange));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"CheckBoxSelection", XMLSettings::EncodeBoolValue(config.checkBoxSelection.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"CloseMainWindowOnTabClose",
		XMLSettings::EncodeBoolValue(config.closeMainWindowOnTabClose));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ConfirmCloseTabs", XMLSettings::EncodeBoolValue(config.confirmCloseTabs));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"DisableFolderSizesNetworkRemovable",
		XMLSettings::EncodeBoolValue(
			config.globalFolderSettings.disableFolderSizesNetworkRemovable));

	wil::com_ptr_nothrow<IXMLDOMElement> displayCentreColorNode;
	XMLSettings::CreateElementNode(xmlDocument, &displayCentreColorNode, settingsNode,
		SETTING_NODE_NAME, L"DisplayCentreColor");
	XMLSettings::AddAttributeToNode(xmlDocument, displayCentreColorNode.get(), L"r",
		XMLSettings::EncodeIntValue(GetRValue(config.displayWindowCentreColor.get())));
	XMLSettings::AddAttributeToNode(xmlDocument, displayCentreColorNode.get(), L"g",
		XMLSettings::EncodeIntValue(GetGValue(config.displayWindowCentreColor.get())));
	XMLSettings::AddAttributeToNode(xmlDocument, displayCentreColorNode.get(), L"b",
		XMLSettings::EncodeIntValue(GetBValue(config.displayWindowCentreColor.get())));

	wil::com_ptr_nothrow<IXMLDOMElement> displayFontNode;
	XMLSettings::CreateElementNode(xmlDocument, &displayFontNode, settingsNode, SETTING_NODE_NAME,
		L"DisplayFont");
	XMLSettings::AddAttributeToNode(xmlDocument, displayFontNode.get(), L"Height",
		XMLSettings::EncodeIntValue(config.displayWindowFont.get().lfHeight));
	XMLSettings::AddAttributeToNode(xmlDocument, displayFontNode.get(), L"Width",
		XMLSettings::EncodeIntValue(config.displayWindowFont.get().lfWidth));
	XMLSettings::AddAttributeToNode(xmlDocument, displayFontNode.get(), L"Weight",
		XMLSettings::EncodeIntValue(config.displayWindowFont.get().lfWeight));
	XMLSettings::AddAttributeToNode(xmlDocument, displayFontNode.get(), L"Italic",
		XMLSettings::EncodeBoolValue(config.displayWindowFont.get().lfItalic));
	XMLSettings::AddAttributeToNode(xmlDocument, displayFontNode.get(), L"Underline",
		XMLSettings::EncodeBoolValue(config.displayWindowFont.get().lfUnderline));
	XMLSettings::AddAttributeToNode(xmlDocument, displayFontNode.get(), L"Strikeout",
		XMLSettings::EncodeBoolValue(config.displayWindowFont.get().lfStrikeOut));
	XMLSettings::AddAttributeToNode(xmlDocument, displayFontNode.get(), L"Font",
		config.displayWindowFont.get().lfFaceName);

	wil::com_ptr_nothrow<IXMLDOMElement> displaySurroundColorNode;
	XMLSettings::CreateElementNode(xmlDocument, &displaySurroundColorNode, settingsNode,
		SETTING_NODE_NAME, L"DisplaySurroundColor");
	XMLSettings::AddAttributeToNode(xmlDocument, displaySurroundColorNode.get(), L"r",
		XMLSettings::EncodeIntValue(GetRValue(config.displayWindowSurroundColor.get())));
	XMLSettings::AddAttributeToNode(xmlDocument, displaySurroundColorNode.get(), L"g",
		XMLSettings::EncodeIntValue(GetGValue(config.displayWindowSurroundColor.get())));
	XMLSettings::AddAttributeToNode(xmlDocument, displaySurroundColorNode.get(), L"b",
		XMLSettings::EncodeIntValue(GetBValue(config.displayWindowSurroundColor.get())));

	wil::com_ptr_nothrow<IXMLDOMElement> displayTextColorNode;
	XMLSettings::CreateElementNode(xmlDocument, &displayTextColorNode, settingsNode,
		SETTING_NODE_NAME, L"DisplayTextColor");
	XMLSettings::AddAttributeToNode(xmlDocument, displayTextColorNode.get(), L"r",
		XMLSettings::EncodeIntValue(GetRValue(config.displayWindowTextColor.get())));
	XMLSettings::AddAttributeToNode(xmlDocument, displayTextColorNode.get(), L"g",
		XMLSettings::EncodeIntValue(GetGValue(config.displayWindowTextColor.get())));
	XMLSettings::AddAttributeToNode(xmlDocument, displayTextColorNode.get(), L"b",
		XMLSettings::EncodeIntValue(GetBValue(config.displayWindowTextColor.get())));

	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"DisplayWindowVertical", XMLSettings::EncodeBoolValue(config.displayWindowVertical));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"DoubleClickTabClose", XMLSettings::EncodeBoolValue(config.doubleClickTabClose));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ExtendTabControl", XMLSettings::EncodeBoolValue(config.extendTabControl.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME, L"ForceSize",
		XMLSettings::EncodeBoolValue(config.globalFolderSettings.forceSize));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"HandleZipFiles", XMLSettings::EncodeBoolValue(config.handleZipFiles));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"HideLinkExtensionGlobal",
		XMLSettings::EncodeBoolValue(config.globalFolderSettings.hideLinkExtension));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"HideSystemFilesGlobal",
		XMLSettings::EncodeBoolValue(config.globalFolderSettings.hideSystemFiles));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME, L"InfoTipType",
		XMLSettings::EncodeIntValue(config.infoTipType));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME, L"InsertSorted",
		XMLSettings::EncodeBoolValue(config.globalFolderSettings.insertSorted));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME, L"Language",
		XMLSettings::EncodeIntValue(config.language));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"LargeToolbarIcons", XMLSettings::EncodeBoolValue(config.useLargeToolbarIcons.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME, L"LockToolbars",
		XMLSettings::EncodeBoolValue(config.lockToolbars.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"NextToCurrent", XMLSettings::EncodeBoolValue(config.openNewTabNextToCurrent));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"NewTabDirectory", config.defaultTabDirectory.c_str());
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"OneClickActivate",
		XMLSettings::EncodeBoolValue(config.globalFolderSettings.oneClickActivate.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"OneClickActivateHoverTime",
		XMLSettings::EncodeIntValue(config.globalFolderSettings.oneClickActivateHoverTime.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"OverwriteExistingFilesConfirmation",
		XMLSettings::EncodeBoolValue(config.overwriteExistingFilesConfirmation));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ReplaceExplorerMode", XMLSettings::EncodeIntValue(config.replaceExplorerMode));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ShowAddressBar", XMLSettings::EncodeBoolValue(config.showAddressBar.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ShowApplicationToolbar",
		XMLSettings::EncodeBoolValue(config.showApplicationToolbar.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ShowBookmarksToolbar", XMLSettings::EncodeBoolValue(config.showBookmarksToolbar.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ShowDrivesToolbar", XMLSettings::EncodeBoolValue(config.showDrivesToolbar.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ShowDisplayWindow", XMLSettings::EncodeBoolValue(config.showDisplayWindow.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ShowExtensions",
		XMLSettings::EncodeBoolValue(config.globalFolderSettings.showExtensions));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ShowFilePreviews", XMLSettings::EncodeBoolValue(config.showFilePreviews));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME, L"ShowFolders",
		XMLSettings::EncodeBoolValue(config.showFolders.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ShowFolderSizes",
		XMLSettings::EncodeBoolValue(config.globalFolderSettings.showFolderSizes));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ShowFriendlyDates",
		XMLSettings::EncodeBoolValue(config.globalFolderSettings.showFriendlyDates));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ShowFullTitlePath", XMLSettings::EncodeBoolValue(config.showFullTitlePath.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ShowGridlinesGlobal",
		XMLSettings::EncodeBoolValue(config.globalFolderSettings.showGridlines.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ShowHiddenGlobal", XMLSettings::EncodeBoolValue(config.defaultFolderSettings.showHidden));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME, L"ShowInfoTips",
		XMLSettings::EncodeBoolValue(config.showInfoTips));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ShowInGroupsGlobal",
		XMLSettings::EncodeBoolValue(config.defaultFolderSettings.showInGroups));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ShowPrivilegeLevelInTitleBar",
		XMLSettings::EncodeBoolValue(config.showPrivilegeLevelInTitleBar.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ShowStatusBar", XMLSettings::EncodeBoolValue(config.showStatusBar));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ShowTabBarAtBottom", XMLSettings::EncodeBoolValue(config.showTabBarAtBottom.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ShowTaskbarThumbnails", XMLSettings::EncodeBoolValue(config.showTaskbarThumbnails));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME, L"ShowToolbar",
		XMLSettings::EncodeBoolValue(config.showMainToolbar.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ShowUserNameTitleBar", XMLSettings::EncodeBoolValue(config.showUserNameInTitleBar.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"SizeDisplayFormat",
		XMLSettings::EncodeIntValue(config.globalFolderSettings.sizeDisplayFormat));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"SortAscendingGlobal",
		XMLSettings::EncodeBoolValue(
			config.defaultFolderSettings.sortDirection == +SortDirection::Ascending));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME, L"StartupMode",
		XMLSettings::EncodeIntValue(config.startupMode));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"SynchronizeTreeview", XMLSettings::EncodeBoolValue(config.synchronizeTreeview.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"TVAutoExpandSelected", XMLSettings::EncodeBoolValue(config.treeViewAutoExpandSelected));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"UseFullRowSelect", XMLSettings::EncodeBoolValue(config.useFullRowSelect.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME, L"IconTheme",
		XMLSettings::EncodeIntValue(config.iconSet));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"TreeViewDelayEnabled", XMLSettings::EncodeBoolValue(config.treeViewDelayEnabled));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ViewModeGlobal", XMLSettings::EncodeIntValue(config.defaultFolderSettings.viewMode));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"CheckPinnedToNamespaceTreeProperty",
		XMLSettings::EncodeBoolValue(config.checkPinnedToNamespaceTreeProperty));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"ShowQuickAccessInTreeView",
		XMLSettings::EncodeBoolValue(config.showQuickAccessInTreeView.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME, L"Theme",
		XMLSettings::EncodeIntValue(config.theme.get()));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"DisplayMixedFilesAndFolders",
		XMLSettings::EncodeBoolValue(config.globalFolderSettings.displayMixedFilesAndFolders));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"UseNaturalSortOrder",
		XMLSettings::EncodeBoolValue(config.globalFolderSettings.useNaturalSortOrder));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"OpenTabsInForeground", XMLSettings::EncodeBoolValue(config.openTabsInForeground));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"GroupSortDirectionGlobal",
		XMLSettings::EncodeIntValue(config.defaultFolderSettings.groupSortDirection));
	XMLSettings::WriteStandardSetting(xmlDocument, settingsNode, SETTING_NODE_NAME,
		L"GoUpOnDoubleClick", XMLSettings::EncodeBoolValue(config.goUpOnDoubleClick));

	auto &mainFont = config.mainFont.get();

	if (mainFont)
	{
		wil::com_ptr_nothrow<IXMLDOMElement> mainFontNode;
		XMLSettings::CreateElementNode(xmlDocument, &mainFontNode, settingsNode, SETTING_NODE_NAME,
			L"MainFont");
		CustomFontStorage::SaveToXml(xmlDocument, mainFontNode.get(), *mainFont);
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

void Save(IXMLDOMDocument *xmlDocument, IXMLDOMNode *rootNode, const Config &config)
{
	wil::com_ptr_nothrow<IXMLDOMElement> settingsNode;
	auto settingsNodeName = wil::make_bstr_nothrow(Storage::CONFIG_FILE_SETTINGS_NODE_NAME);
	HRESULT hr = xmlDocument->createElement(settingsNodeName.get(), &settingsNode);

	if (hr != S_OK)
	{
		return;
	}

	SaveToNode(xmlDocument, settingsNode.get(), config);

	XMLSettings::AppendChildToParent(settingsNode.get(), rootNode);
}

}
