// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainToolbarStorage.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/XMLSettings.h"
#include <boost/bimap.hpp>
#include <wil/com.h>
#include <format>

namespace
{

using ToolbarButtonMapping = boost::bimap<MainToolbarButton, std::wstring>;

// This matches the ID value used for a separator by TB_SAVERESTORE.
const DWORD REGISTRY_SEPARATOR_STORAGE_ID = static_cast<DWORD>(-1);

// clang-format off
const ToolbarButtonMapping::value_type g_xmlButtonMappingValues[] = {
	{ MainToolbarButton::Back, L"Back" },
	{ MainToolbarButton::Forward, L"Forward" },
	{ MainToolbarButton::Up, L"Up" },
	{ MainToolbarButton::Folders, L"Folders" },
	{ MainToolbarButton::CopyTo, L"Copy To" },
	{ MainToolbarButton::MoveTo, L"Move To" },
	{ MainToolbarButton::NewFolder, L"New Folder" },
	{ MainToolbarButton::Copy, L"Copy" },
	{ MainToolbarButton::Cut, L"Cut" },
	{ MainToolbarButton::Paste, L"Paste" },
	{ MainToolbarButton::Delete, L"Delete" },
	{ MainToolbarButton::Views, L"Views" },
	{ MainToolbarButton::Search, L"Search" },
	{ MainToolbarButton::Properties, L"Properties" },
	{ MainToolbarButton::Refresh, L"Refresh" },
	{ MainToolbarButton::AddBookmark, L"Bookmark the current tab" },
	{ MainToolbarButton::NewTab, L"Create a new tab" },
	{ MainToolbarButton::OpenCommandPrompt, L"Open Command Prompt" },
	{ MainToolbarButton::Bookmarks, L"Organize Bookmarks" },
	{ MainToolbarButton::DeletePermanently, L"Delete Permanently" },
	{ MainToolbarButton::SplitFile, L"Split File" },
	{ MainToolbarButton::MergeFiles, L"Merge Files" },
	{ MainToolbarButton::CloseTab, L"Close Tab" },

	{ MainToolbarButton::Separator, L"Separator" }
};
// clang-format on

static_assert(std::size(g_xmlButtonMappingValues) == MainToolbarButton::_size());

// Ideally, toolbar button IDs would be saved in the XML config file, rather than button strings,
// but changing that would at least break forward compatibility.
const ToolbarButtonMapping XML_BUTTON_NAME_MAPPINGS(std::begin(g_xmlButtonMappingValues),
	std::end(g_xmlButtonMappingValues));

constexpr WCHAR XM_BUTTON_ATTRIBUTE_TEMPLATE[] = L"Button{}";

}

namespace MainToolbarStorage
{

void MainToolbarButtons::AddButton(MainToolbarButton button)
{
	if ((button != +MainToolbarButton::Separator)
		&& (std::find(m_buttons.begin(), m_buttons.end(), button) != m_buttons.end()))
	{
		return;
	}

	m_buttons.push_back(button);
}

const std::vector<MainToolbarButton> &MainToolbarButtons::GetButtons() const
{
	return m_buttons;
}

std::optional<MainToolbarButtons> LoadFromRegistry(HKEY parentKey, const std::wstring &valueName)
{
	std::vector<DWORD> buttonsData;
	auto res = RegistrySettings::ReadVectorFromBinaryValue(parentKey, valueName, buttonsData);

	if (res != ERROR_SUCCESS)
	{
		return std::nullopt;
	}

	MainToolbarButtons buttons;

	for (auto &loadedButtonType : buttonsData)
	{
		if (loadedButtonType == REGISTRY_SEPARATOR_STORAGE_ID)
		{
			loadedButtonType = MainToolbarButton::Separator;
		}

		auto buttonType = MainToolbarButton::_from_integral_nothrow(loadedButtonType);

		if (!buttonType)
		{
			continue;
		}

		buttons.AddButton(*buttonType);
	}

	return buttons;
}

void SaveToRegistry(HKEY parentKey, const std::wstring &valueName,
	const MainToolbarButtons &buttons)
{
	// Data is saved here in the same format as TB_SAVERESTORE. That is, as a set of DWORDs, with
	// each DWORD containing the ID for the button at that index.
	std::vector<DWORD> buttonsData;
	buttonsData.reserve(buttons.GetButtons().size());

	std::transform(buttons.GetButtons().begin(), buttons.GetButtons().end(),
		std::back_inserter(buttonsData),
		[](MainToolbarButton buttonType)
		{
			return (buttonType == +MainToolbarButton::Separator) ? REGISTRY_SEPARATOR_STORAGE_ID
																 : buttonType;
		});

	RegistrySettings::SaveVectorToBinaryValue(parentKey, valueName, buttonsData);
}

std::optional<MainToolbarButtons> LoadFromXml(IXMLDOMNode *toolbarNode)
{
	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> attributeMap;
	HRESULT hr = toolbarNode->get_attributes(&attributeMap);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	MainToolbarButtons buttons;

	for (int index = 0;; index++)
	{
		auto buttonAttributeName = std::format(XM_BUTTON_ATTRIBUTE_TEMPLATE, index);

		std::wstring buttonName;
		hr = NXMLSettings::GetStringFromMap(attributeMap.get(), buttonAttributeName, buttonName);

		if (FAILED(hr))
		{
			break;
		}

		auto itr = XML_BUTTON_NAME_MAPPINGS.right.find(buttonName);

		if (itr == XML_BUTTON_NAME_MAPPINGS.right.end())
		{
			continue;
		}

		buttons.AddButton(itr->second);
	}

	return buttons;
}

void SaveToXml(IXMLDOMDocument *xmlDocument, IXMLDOMElement *toolbarNode,
	const MainToolbarButtons &buttons)
{
	int index = 0;

	for (auto button : buttons.GetButtons())
	{
		auto buttonAttributeName = std::format(XM_BUTTON_ATTRIBUTE_TEMPLATE, index);
		auto buttonName = XML_BUTTON_NAME_MAPPINGS.left.at(button);
		NXMLSettings::AddAttributeToNode(xmlDocument, toolbarNode, buttonAttributeName.c_str(),
			buttonName.c_str());

		index++;
	}
}

}
