// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainRebarXmlStorage.h"
#include "MainRebarStorage.h"
#include "../Helper/XMLSettings.h"
#include <optional>

namespace
{

const wchar_t TOOLBAR_NODE_NAME[] = L"Toolbar";

const wchar_t SETTING_ID[] = L"id";
const wchar_t SETTING_STYLE[] = L"Style";
const wchar_t SETTING_LENGTH[] = L"Length";

std::optional<RebarBandStorageInfo> LoadRebarBandInfo(IXMLDOMNode *parentNode)
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

	RebarBandStorageInfo bandInfo;
	bandInfo.id = id;
	bandInfo.style = style;
	bandInfo.length = length;
	return bandInfo;
}

void SaveRebarBandInfo(IXMLDOMDocument *xmlDocument, IXMLDOMElement *toolbarNode,
	const RebarBandStorageInfo &bandInfo)
{
	XMLSettings::AddAttributeToNode(xmlDocument, toolbarNode, SETTING_ID,
		XMLSettings::EncodeIntValue(bandInfo.id));
	XMLSettings::AddAttributeToNode(xmlDocument, toolbarNode, SETTING_STYLE,
		XMLSettings::EncodeIntValue(bandInfo.style));
	XMLSettings::AddAttributeToNode(xmlDocument, toolbarNode, SETTING_LENGTH,
		XMLSettings::EncodeIntValue(bandInfo.length));
}

}

namespace MainRebarXmlStorage
{

std::vector<RebarBandStorageInfo> Load(IXMLDOMNode *mainRebarNode)
{
	return XMLSettings::ReadItemList<RebarBandStorageInfo>(mainRebarNode, TOOLBAR_NODE_NAME,
		LoadRebarBandInfo);
}

void Save(IXMLDOMDocument *xmlDocument, IXMLDOMElement *mainRebarNode,
	const std::vector<RebarBandStorageInfo> &rebarStorageInfo)
{
	XMLSettings::SaveItemList<RebarBandStorageInfo>(xmlDocument, mainRebarNode, rebarStorageInfo,
		TOOLBAR_NODE_NAME, SaveRebarBandInfo);
}

}
