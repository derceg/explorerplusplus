// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "CustomFontStorage.h"
#include "CustomFont.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/XMLSettings.h"
#include <wil/com.h>

constexpr TCHAR SETTING_NAME[] = _T("Name");
constexpr TCHAR SETTING_SIZE[] = _T("Size");

std::unique_ptr<CustomFont> LoadCustomFontFromKey(HKEY key);
void SaveCustomFontToKey(HKEY key, const CustomFont &customFont);

std::unique_ptr<CustomFont> LoadCustomFontFromRegistry(const std::wstring &keyPath)
{
	wil::unique_hkey fontKey;
	LSTATUS res = RegOpenKeyEx(HKEY_CURRENT_USER, keyPath.c_str(), 0, KEY_READ, &fontKey);

	if (res == ERROR_SUCCESS)
	{
		return LoadCustomFontFromKey(fontKey.get());
	}

	return nullptr;
}

std::unique_ptr<CustomFont> LoadCustomFontFromKey(HKEY key)
{
	std::wstring name;
	LSTATUS result = RegistrySettings::ReadString(key, SETTING_NAME, name);

	if (result != ERROR_SUCCESS)
	{
		return nullptr;
	}

	int size;
	result = RegistrySettings::Read32BitValueFromRegistry(key, SETTING_SIZE, size);

	if (result != ERROR_SUCCESS)
	{
		return nullptr;
	}

	return std::make_unique<CustomFont>(name, size);
}

void SaveCustomFontToRegistry(const std::wstring &keyPath, const CustomFont &customFont)
{
	wil::unique_hkey fontKey;
	LSTATUS res = RegCreateKeyEx(HKEY_CURRENT_USER, keyPath.c_str(), 0, nullptr,
		REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &fontKey, nullptr);

	if (res == ERROR_SUCCESS)
	{
		SaveCustomFontToKey(fontKey.get(), customFont);
	}
}

void SaveCustomFontToKey(HKEY key, const CustomFont &customFont)
{
	RegistrySettings::SaveString(key, SETTING_NAME, customFont.GetName());
	RegistrySettings::SaveDword(key, SETTING_SIZE, customFont.GetSize());
}

std::unique_ptr<CustomFont> LoadCustomFontFromXml(IXMLDOMNode *parentNode)
{
	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> attributeMap;
	HRESULT hr = parentNode->get_attributes(&attributeMap);

	if (hr != S_OK)
	{
		return nullptr;
	}

	std::wstring name;
	hr = NXMLSettings::GetStringFromMap(attributeMap.get(), SETTING_NAME, name);

	if (hr != S_OK)
	{
		return nullptr;
	}

	int size;
	hr = NXMLSettings::GetIntFromMap(attributeMap.get(), SETTING_SIZE, size);

	if (hr != S_OK)
	{
		return nullptr;
	}

	return std::make_unique<CustomFont>(name, size);
}

void SaveCustomFontToXml(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const CustomFont &customFont)
{
	NXMLSettings::AddAttributeToNode(xmlDocument, parentNode, SETTING_NAME,
		customFont.GetName().c_str());
	NXMLSettings::AddAttributeToNode(xmlDocument, parentNode, SETTING_SIZE,
		NXMLSettings::EncodeIntValue(customFont.GetSize()));
}
