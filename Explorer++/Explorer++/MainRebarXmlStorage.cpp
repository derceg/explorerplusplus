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

void SaveRebarBandInfo(IXMLDOMDocument *xmlDocument, IXMLDOMNode *mainRebarNode,
	const RebarBandStorageInfo &bandInfo)
{
	wil::com_ptr_nothrow<IXMLDOMElement> toolbarNode;
	auto toolbarNodeName = wil::make_bstr_nothrow(TOOLBAR_NODE_NAME);
	HRESULT hr = xmlDocument->createElement(toolbarNodeName.get(), &toolbarNode);

	if (hr != S_OK)
	{
		return;
	}

	XMLSettings::AddAttributeToNode(xmlDocument, toolbarNode.get(), SETTING_ID,
		XMLSettings::EncodeIntValue(bandInfo.id));
	XMLSettings::AddAttributeToNode(xmlDocument, toolbarNode.get(), SETTING_STYLE,
		XMLSettings::EncodeIntValue(bandInfo.style));
	XMLSettings::AddAttributeToNode(xmlDocument, toolbarNode.get(), SETTING_LENGTH,
		XMLSettings::EncodeIntValue(bandInfo.length));

	XMLSettings::AppendChildToParent(toolbarNode.get(), mainRebarNode);
}

}

namespace MainRebarXmlStorage
{

std::vector<RebarBandStorageInfo> Load(IXMLDOMNode *mainRebarNode)
{
	wil::com_ptr_nothrow<IXMLDOMNode> childNode;
	auto queryString = wil::make_bstr_nothrow(TOOLBAR_NODE_NAME);

	wil::com_ptr_nothrow<IXMLDOMNodeList> childNodes;
	HRESULT hr = mainRebarNode->selectNodes(queryString.get(), &childNodes);

	if (FAILED(hr))
	{
		return {};
	}

	std::vector<RebarBandStorageInfo> rebarStorageInfo;

	while (childNodes->nextNode(&childNode) == S_OK)
	{
		auto bandInfo = LoadRebarBandInfo(childNode.get());

		if (bandInfo)
		{
			rebarStorageInfo.push_back(*bandInfo);
		}
	}

	return rebarStorageInfo;
}

void Save(IXMLDOMDocument *xmlDocument, IXMLDOMNode *mainRebarNode,
	const std::vector<RebarBandStorageInfo> &rebarStorageInfo)
{
	for (const auto &bandStorageInfo : rebarStorageInfo)
	{
		SaveRebarBandInfo(xmlDocument, mainRebarNode, bandStorageInfo);
	}
}

}
