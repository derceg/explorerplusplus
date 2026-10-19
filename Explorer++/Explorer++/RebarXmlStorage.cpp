// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RebarXmlStorage.h"
#include "RebarBandStorageData.h"
#include "../Helper/XMLSettings.h"
#include <optional>

namespace
{

const wchar_t TOOLBAR_NODE_NAME[] = L"Toolbar";

const wchar_t SETTING_ID[] = L"id";
const wchar_t SETTING_STYLE[] = L"Style";
const wchar_t SETTING_LENGTH[] = L"Length";

std::optional<RebarBandStorageData> LoadRebarBandInfo(IXMLDOMNode *parentNode)
{
	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> attributeMap;
	HRESULT hr = parentNode->get_attributes(&attributeMap);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	int id;
	hr = XMLSettings::GetIntFromMap(attributeMap.get(), SETTING_ID, id);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	int style;
	hr = XMLSettings::GetIntFromMap(attributeMap.get(), SETTING_STYLE, style);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	int length;
	hr = XMLSettings::GetIntFromMap(attributeMap.get(), SETTING_LENGTH, length);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	RebarBandStorageData bandInfo;
	bandInfo.id = id;
	bandInfo.style = style;
	bandInfo.length = length;
	return bandInfo;
}

void SaveRebarBandInfo(IXMLDOMDocument *xmlDocument, IXMLDOMElement *toolbarNode,
	const RebarBandStorageData &bandInfo)
{
	XMLSettings::AddAttributeToNode(xmlDocument, toolbarNode, SETTING_ID,
		XMLSettings::EncodeIntValue(bandInfo.id));
	XMLSettings::AddAttributeToNode(xmlDocument, toolbarNode, SETTING_STYLE,
		XMLSettings::EncodeIntValue(bandInfo.style));
	XMLSettings::AddAttributeToNode(xmlDocument, toolbarNode, SETTING_LENGTH,
		XMLSettings::EncodeIntValue(bandInfo.length));
}

}

namespace RebarXmlStorage
{

std::vector<RebarBandStorageData> Load(IXMLDOMNode *rebarNode)
{
	return XMLSettings::ReadItemList<RebarBandStorageData>(rebarNode, TOOLBAR_NODE_NAME,
		LoadRebarBandInfo);
}

void Save(IXMLDOMDocument *xmlDocument, IXMLDOMElement *rebarNode,
	const std::vector<RebarBandStorageData> &rebarStorageInfo)
{
	XMLSettings::SaveItemList<RebarBandStorageData>(xmlDocument, rebarNode, rebarStorageInfo,
		TOOLBAR_NODE_NAME, SaveRebarBandInfo);
}

}
