// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabXmlStorage.h"
#include "ColumnXmlStorage.h"
#include "TabStorage.h"
#include "../Helper/XMLSettings.h"
#include <wil/com.h>
#include <msxml.h>
#include <optional>

using namespace std::string_literals;

namespace TabXmlStorage
{

namespace
{

const WCHAR TAB_NODE_NAME[] = L"Tab";

// Required values
const WCHAR SETTING_DIRECTORY[] = L"Directory";

// Folder settings
const WCHAR SETTING_VIEW_MODE[] = L"ViewMode";
const WCHAR SETTING_SORT_MODE[] = L"SortMode";
const WCHAR SETTING_SORT_ASCENDING[] = L"SortAscending";
const WCHAR SETTING_GROUP_MODE[] = L"GroupMode";
const WCHAR SETTING_GROUP_SORT_DIRECTION[] = L"GroupSortDirection";
const WCHAR SETTING_SHOW_IN_GROUPS[] = L"ShowInGroups";
const WCHAR SETTING_APPLY_FILTER[] = L"ApplyFilter";
const WCHAR SETTING_FILTER_CASE_SENSITIVE[] = L"FilterCaseSensitive";
const WCHAR SETTING_SHOW_HIDDEN[] = L"ShowHidden";
const WCHAR SETTING_AUTO_ARRANGE[] = L"AutoArrange";
const WCHAR SETTING_FILTER[] = L"Filter";

// Columns
const WCHAR SETTING_COLUMNS[] = L"Columns";

// Tab settings
const WCHAR SETTING_TAB_LOCKED[] = L"Locked";
const WCHAR SETTING_TAB_ADDRESS_LOCKED[] = L"AddressLocked";
const WCHAR SETTING_TAB_CUSTOM_NAME[] = L"CustomName";

std::optional<std::wstring> MaybeLoadDirectory(IXMLDOMNode *tabNode)
{
	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> attributeMap;
	HRESULT hr = tabNode->get_attributes(&attributeMap);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	std::wstring directory;
	hr = NXMLSettings::GetStringFromMap(attributeMap.get(), SETTING_DIRECTORY, directory);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	return directory;
}

template <BetterEnum T>
void LoadBetterEnumValue(IXMLDOMNamedNodeMap *attributeMap, const std::wstring &valueName,
	T &output)
{
	int value;
	HRESULT hr = NXMLSettings::GetIntFromMap(attributeMap, valueName, value);

	if (FAILED(hr) || !T::_is_valid(value))
	{
		return;
	}

	output = T::_from_integral(value);
}

void LoadBooleanSortDirection(IXMLDOMNamedNodeMap *attributeMap, const std::wstring &valueName,
	SortDirection &output)
{
	bool sortAscending;
	HRESULT hr = NXMLSettings::GetBoolFromMap(attributeMap, valueName, sortAscending);

	if (FAILED(hr))
	{
		return;
	}

	output = sortAscending ? SortDirection::Ascending : SortDirection::Descending;
}

FolderSettings LoadFolderSettings(IXMLDOMNode *tabNode)
{
	FolderSettings folderSettings;

	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> attributeMap;
	HRESULT hr = tabNode->get_attributes(&attributeMap);

	if (FAILED(hr))
	{
		return folderSettings;
	}

	LoadBetterEnumValue(attributeMap.get(), SETTING_VIEW_MODE, folderSettings.viewMode);
	LoadBetterEnumValue(attributeMap.get(), SETTING_SORT_MODE, folderSettings.sortMode);
	folderSettings.groupMode = folderSettings.sortMode;
	LoadBooleanSortDirection(attributeMap.get(), SETTING_SORT_ASCENDING,
		folderSettings.sortDirection);
	folderSettings.groupSortDirection = folderSettings.sortDirection;
	LoadBetterEnumValue(attributeMap.get(), SETTING_GROUP_MODE, folderSettings.groupMode);
	LoadBetterEnumValue(attributeMap.get(), SETTING_GROUP_SORT_DIRECTION,
		folderSettings.groupSortDirection);
	NXMLSettings::GetBoolFromMap(attributeMap.get(), SETTING_SHOW_IN_GROUPS,
		folderSettings.showInGroups);
	NXMLSettings::GetBoolFromMap(attributeMap.get(), SETTING_APPLY_FILTER,
		folderSettings.applyFilter);
	NXMLSettings::GetBoolFromMap(attributeMap.get(), SETTING_FILTER_CASE_SENSITIVE,
		folderSettings.filterCaseSensitive);
	NXMLSettings::GetBoolFromMap(attributeMap.get(), SETTING_SHOW_HIDDEN,
		folderSettings.showHidden);
	NXMLSettings::GetBoolFromMap(attributeMap.get(), SETTING_AUTO_ARRANGE,
		folderSettings.autoArrange);
	NXMLSettings::GetStringFromMap(attributeMap.get(), SETTING_FILTER, folderSettings.filter);

	return folderSettings;
}

FolderColumns LoadColumns(IXMLDOMNode *tabNode)
{
	FolderColumns columns;

	wil::com_ptr_nothrow<IXMLDOMNode> columnsNode;
	auto queryString = wil::make_bstr_nothrow((L"./"s + SETTING_COLUMNS).c_str());
	HRESULT hr = tabNode->selectSingleNode(queryString.get(), &columnsNode);

	if (SUCCEEDED(hr))
	{
		ColumnXmlStorage::LoadAllColumnSets(columnsNode.get(), columns);
	}

	return columns;
}

TabSettings LoadTabSettings(IXMLDOMNode *tabNode)
{
	TabSettings tabSettings;

	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> attributeMap;
	HRESULT hr = tabNode->get_attributes(&attributeMap);

	if (FAILED(hr))
	{
		return tabSettings;
	}

	bool locked = false;
	NXMLSettings::GetBoolFromMap(attributeMap.get(), SETTING_TAB_LOCKED, locked);

	bool addressLocked = false;
	NXMLSettings::GetBoolFromMap(attributeMap.get(), SETTING_TAB_ADDRESS_LOCKED, addressLocked);

	if (addressLocked)
	{
		tabSettings.lockState = Tab::LockState::AddressLocked;
	}
	else if (locked)
	{
		tabSettings.lockState = Tab::LockState::Locked;
	}

	std::wstring customName;
	NXMLSettings::GetStringFromMap(attributeMap.get(), SETTING_TAB_CUSTOM_NAME, customName);
	tabSettings.name = customName;

	return tabSettings;
}

std::optional<TabStorageData> LoadTabInfo(IXMLDOMNode *tabNode)
{
	auto directory = MaybeLoadDirectory(tabNode);

	if (!directory)
	{
		return std::nullopt;
	}

	TabStorageData tabStorageData;
	tabStorageData.directory = *directory;
	tabStorageData.folderSettings = LoadFolderSettings(tabNode);
	tabStorageData.columns = LoadColumns(tabNode);
	tabStorageData.tabSettings = LoadTabSettings(tabNode);
	return tabStorageData;
}

void SaveFolderSettings(IXMLDOMDocument *xmlDocument, IXMLDOMElement *tabNode,
	const FolderSettings &folderSettings)
{
	NXMLSettings::AddAttributeToNode(xmlDocument, tabNode, SETTING_VIEW_MODE,
		NXMLSettings::EncodeIntValue(folderSettings.viewMode));
	NXMLSettings::AddAttributeToNode(xmlDocument, tabNode, SETTING_SORT_MODE,
		NXMLSettings::EncodeIntValue(folderSettings.sortMode));

	// For backwards compatibility, the value saved here is a bool.
	NXMLSettings::AddAttributeToNode(xmlDocument, tabNode, SETTING_SORT_ASCENDING,
		NXMLSettings::EncodeBoolValue(folderSettings.sortDirection == +SortDirection::Ascending));

	NXMLSettings::AddAttributeToNode(xmlDocument, tabNode, SETTING_GROUP_MODE,
		NXMLSettings::EncodeIntValue(folderSettings.groupMode));
	NXMLSettings::AddAttributeToNode(xmlDocument, tabNode, SETTING_GROUP_SORT_DIRECTION,
		NXMLSettings::EncodeIntValue(folderSettings.groupSortDirection));
	NXMLSettings::AddAttributeToNode(xmlDocument, tabNode, SETTING_SHOW_IN_GROUPS,
		NXMLSettings::EncodeBoolValue(folderSettings.showInGroups));
	NXMLSettings::AddAttributeToNode(xmlDocument, tabNode, SETTING_APPLY_FILTER,
		NXMLSettings::EncodeBoolValue(folderSettings.applyFilter));
	NXMLSettings::AddAttributeToNode(xmlDocument, tabNode, SETTING_FILTER_CASE_SENSITIVE,
		NXMLSettings::EncodeBoolValue(folderSettings.filterCaseSensitive));
	NXMLSettings::AddAttributeToNode(xmlDocument, tabNode, SETTING_SHOW_HIDDEN,
		NXMLSettings::EncodeBoolValue(folderSettings.showHidden));
	NXMLSettings::AddAttributeToNode(xmlDocument, tabNode, SETTING_AUTO_ARRANGE,
		NXMLSettings::EncodeBoolValue(folderSettings.autoArrange));
	NXMLSettings::AddAttributeToNode(xmlDocument, tabNode, SETTING_FILTER,
		folderSettings.filter.c_str());
}

void SaveColumns(IXMLDOMDocument *xmlDocument, IXMLDOMElement *tabNode,
	const FolderColumns &columns)
{
	wil::com_ptr_nothrow<IXMLDOMElement> columnsNode;
	auto bstr = wil::make_bstr_nothrow(SETTING_COLUMNS);
	HRESULT hr = xmlDocument->createElement(bstr.get(), &columnsNode);

	if (FAILED(hr))
	{
		return;
	}

	ColumnXmlStorage::SaveAllColumnSets(xmlDocument, columnsNode.get(), columns);

	NXMLSettings::AppendChildToParent(columnsNode.get(), tabNode);
}

void SaveTabSettings(IXMLDOMDocument *xmlDocument, IXMLDOMElement *tabNode,
	const TabSettings &tabSettings)
{
	NXMLSettings::AddAttributeToNode(xmlDocument, tabNode, SETTING_TAB_LOCKED,
		NXMLSettings::EncodeBoolValue(tabSettings.lockState == Tab::LockState::Locked));
	NXMLSettings::AddAttributeToNode(xmlDocument, tabNode, SETTING_TAB_ADDRESS_LOCKED,
		NXMLSettings::EncodeBoolValue(tabSettings.lockState == Tab::LockState::AddressLocked));

	std::wstring customName = tabSettings.name ? *tabSettings.name : L"";
	NXMLSettings::AddAttributeToNode(xmlDocument, tabNode, SETTING_TAB_CUSTOM_NAME,
		customName.c_str());
}

void SaveTabInfo(IXMLDOMDocument *xmlDocument, IXMLDOMElement *tabsNode, const TabStorageData &tab)
{
	wil::com_ptr_nothrow<IXMLDOMElement> tabNode;
	NXMLSettings::CreateElementNode(xmlDocument, &tabNode, tabsNode, TAB_NODE_NAME, L"");

	NXMLSettings::AddAttributeToNode(xmlDocument, tabNode.get(), SETTING_DIRECTORY,
		tab.directory.c_str());

	SaveFolderSettings(xmlDocument, tabNode.get(), tab.folderSettings);
	SaveColumns(xmlDocument, tabNode.get(), tab.columns);
	SaveTabSettings(xmlDocument, tabNode.get(), tab.tabSettings);
}

}

std::vector<TabStorageData> Load(IXMLDOMNode *tabsNode)
{
	auto queryString = wil::make_bstr_nothrow((L"./"s + TAB_NODE_NAME).c_str());
	wil::com_ptr_nothrow<IXMLDOMNodeList> childNodes;
	HRESULT hr = tabsNode->selectNodes(queryString.get(), &childNodes);

	if (FAILED(hr))
	{
		return {};
	}

	wil::com_ptr_nothrow<IXMLDOMNode> childNode;
	std::vector<TabStorageData> tabs;

	while (childNodes->nextNode(&childNode) == S_OK)
	{
		auto tab = LoadTabInfo(childNode.get());

		if (tab)
		{
			tabs.push_back(*tab);
		}
	}

	return tabs;
}

void Save(IXMLDOMDocument *xmlDocument, IXMLDOMElement *tabsNode,
	const std::vector<TabStorageData> &tabs)
{
	for (const auto &tab : tabs)
	{
		SaveTabInfo(xmlDocument, tabsNode, tab);
	}
}

}
