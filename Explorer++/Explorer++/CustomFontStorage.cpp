// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "CustomFontStorage.h"
#include "CustomFont.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/XMLSettings.h"
#include <wil/com.h>

namespace
{

constexpr wchar_t SETTING_NAME[] = L"Name";
constexpr wchar_t SETTING_SIZE[] = L"Size";

}

namespace CustomFontStorage
{

std::unique_ptr<CustomFont> LoadFromRegistry(HKEY fontKey)
{
	std::wstring name;
	LSTATUS result = RegistrySettings::ReadString(fontKey, SETTING_NAME, name);

	if (result != ERROR_SUCCESS)
	{
		return nullptr;
	}

	int size;
	result = RegistrySettings::Read32BitValueFromRegistry(fontKey, SETTING_SIZE, size);

	if (result != ERROR_SUCCESS)
	{
		return nullptr;
	}

	return std::make_unique<CustomFont>(name, size);
}

void SaveToRegistry(HKEY fontKey, const CustomFont &customFont)
{
	RegistrySettings::SaveString(fontKey, SETTING_NAME, customFont.GetName());
	RegistrySettings::SaveDword(fontKey, SETTING_SIZE, customFont.GetSize());
}

std::unique_ptr<CustomFont> LoadFromXml(IXMLDOMNode *fontNode)
{
	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> attributeMap;
	HRESULT hr = fontNode->get_attributes(&attributeMap);

	if (hr != S_OK)
	{
		return nullptr;
	}

	std::wstring name;
	hr = XMLSettings::GetStringFromMap(attributeMap.get(), SETTING_NAME, name);

	if (hr != S_OK)
	{
		return nullptr;
	}

	int size;
	hr = XMLSettings::GetIntFromMap(attributeMap.get(), SETTING_SIZE, size);

	if (hr != S_OK)
	{
		return nullptr;
	}

	return std::make_unique<CustomFont>(name, size);
}

void SaveToXml(IXMLDOMDocument *xmlDocument, IXMLDOMElement *fontNode, const CustomFont &customFont)
{
	XMLSettings::AddAttributeToNode(xmlDocument, fontNode, SETTING_NAME,
		customFont.GetName().c_str());
	XMLSettings::AddAttributeToNode(xmlDocument, fontNode, SETTING_SIZE,
		XMLSettings::EncodeIntValue(customFont.GetSize()));
}

}
