// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainToolbarStorage.h"
#include "MainToolbarLayout.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/XMLSettings.h"
#include <boost/bimap.hpp>
#include <magic_enum/magic_enum.hpp>
#include <wil/com.h>
#include <algorithm>
#include <format>

namespace
{

using ToolbarButtonMapping = boost::bimap<MainToolbarButtonId, std::wstring>;

// This matches the ID value used for a separator by TB_SAVERESTORE.
const DWORD REGISTRY_SEPARATOR_STORAGE_ID = static_cast<DWORD>(-1);

const ToolbarButtonMapping::value_type g_xmlButtonMappingValues[] = {
	{ MainToolbarButtonId::Back, L"Back" },
	{ MainToolbarButtonId::Forward, L"Forward" },
	{ MainToolbarButtonId::Up, L"Up" },
	{ MainToolbarButtonId::Folders, L"Folders" },
	{ MainToolbarButtonId::CopyTo, L"Copy To" },
	{ MainToolbarButtonId::MoveTo, L"Move To" },
	{ MainToolbarButtonId::NewFolder, L"New Folder" },
	{ MainToolbarButtonId::Copy, L"Copy" },
	{ MainToolbarButtonId::Cut, L"Cut" },
	{ MainToolbarButtonId::Paste, L"Paste" },
	{ MainToolbarButtonId::Delete, L"Delete" },
	{ MainToolbarButtonId::Views, L"Views" },
	{ MainToolbarButtonId::Search, L"Search" },
	{ MainToolbarButtonId::Properties, L"Properties" },
	{ MainToolbarButtonId::Refresh, L"Refresh" },
	{ MainToolbarButtonId::AddBookmark, L"Bookmark the current tab" },
	{ MainToolbarButtonId::NewTab, L"Create a new tab" },
	{ MainToolbarButtonId::OpenCommandPrompt, L"Open Command Prompt" },
	{ MainToolbarButtonId::Bookmarks, L"Organize Bookmarks" },
	{ MainToolbarButtonId::DeletePermanently, L"Delete Permanently" },
	{ MainToolbarButtonId::SplitFile, L"Split File" },
	{ MainToolbarButtonId::MergeFiles, L"Merge Files" },
	{ MainToolbarButtonId::CloseTab, L"Close Tab" },

	{ MainToolbarButtonId::Separator, L"Separator" },
};

static_assert(std::size(g_xmlButtonMappingValues) == magic_enum::enum_count<MainToolbarButtonId>());

// Ideally, toolbar button IDs would be saved in the XML config file, rather than button strings,
// but changing that would at least break forward compatibility.
const ToolbarButtonMapping XML_BUTTON_NAME_MAPPINGS(std::begin(g_xmlButtonMappingValues),
	std::end(g_xmlButtonMappingValues));

constexpr WCHAR XM_BUTTON_ATTRIBUTE_TEMPLATE[] = L"Button{}";

}

namespace MainToolbarStorage
{

std::optional<MainToolbarLayout> LoadFromRegistry(HKEY parentKey, const std::wstring &valueName)
{
	std::vector<DWORD> buttonsData;
	auto res = RegistrySettings::ReadVectorFromBinaryValue(parentKey, valueName, buttonsData);

	if (res != ERROR_SUCCESS)
	{
		return std::nullopt;
	}

	MainToolbarLayout layout;

	for (auto &loadedButtonType : buttonsData)
	{
		if (loadedButtonType == REGISTRY_SEPARATOR_STORAGE_ID)
		{
			layout.AddButton(MainToolbarButtonId::Separator);
		}
		else
		{
			auto buttonType = magic_enum::enum_cast<MainToolbarButtonId>(loadedButtonType);

			if (!buttonType)
			{
				continue;
			}

			layout.AddButton(*buttonType);
		}
	}

	return layout;
}

void SaveToRegistry(HKEY parentKey, const std::wstring &valueName, const MainToolbarLayout &layout)
{
	// Data is saved here in the same format as TB_SAVERESTORE. That is, as a set of DWORDs, with
	// each DWORD containing the ID for the button at that index.
	std::vector<DWORD> buttonsData;
	buttonsData.reserve(layout.GetButtons().size());

	std::ranges::transform(layout.GetButtons(), std::back_inserter(buttonsData),
		[](MainToolbarButtonId buttonType)
		{
			return (buttonType == MainToolbarButtonId::Separator) ? REGISTRY_SEPARATOR_STORAGE_ID
																  : static_cast<DWORD>(buttonType);
		});

	RegistrySettings::SaveVectorToBinaryValue(parentKey, valueName, buttonsData);
}

std::optional<MainToolbarLayout> LoadFromXml(IXMLDOMNode *toolbarNode)
{
	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> attributeMap;
	HRESULT hr = toolbarNode->get_attributes(&attributeMap);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	MainToolbarLayout layout;

	for (int index = 0;; index++)
	{
		auto buttonAttributeName = std::format(XM_BUTTON_ATTRIBUTE_TEMPLATE, index);

		std::wstring buttonName;
		hr = XMLSettings::GetStringFromMap(attributeMap.get(), buttonAttributeName, buttonName);

		if (FAILED(hr))
		{
			break;
		}

		auto itr = XML_BUTTON_NAME_MAPPINGS.right.find(buttonName);

		if (itr == XML_BUTTON_NAME_MAPPINGS.right.end())
		{
			continue;
		}

		layout.AddButton(itr->second);
	}

	return layout;
}

void SaveToXml(IXMLDOMDocument *xmlDocument, IXMLDOMElement *toolbarNode,
	const MainToolbarLayout &layout)
{
	int index = 0;

	for (auto button : layout.GetButtons())
	{
		auto buttonAttributeName = std::format(XM_BUTTON_ATTRIBUTE_TEMPLATE, index);
		auto buttonName = XML_BUTTON_NAME_MAPPINGS.left.at(button);
		XMLSettings::AddAttributeToNode(xmlDocument, toolbarNode, buttonAttributeName.c_str(),
			buttonName.c_str());

		index++;
	}
}

}
