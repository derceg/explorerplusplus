// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainRebarXmlStorage.h"
#include "MainRebarStorage.h"
#include "../Helper/XMLSettings.h"
#include <optional>

namespace MainRebarXmlStorage
{

namespace
{

const TCHAR MAIN_REBAR_NODE_NAME[] = _T("Toolbars");

const WCHAR SETTING_ID[] = L"id";
const WCHAR SETTING_STYLE[] = L"Style";
const WCHAR SETTING_LENGTH[] = L"Length";

std::optional<RebarBandStorageInfo> LoadRebarBandInfo(IXMLDOMNode *parentNode)
{
	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> attributeMap;
	HRESULT hr = parentNode->get_attributes(&attributeMap);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	int id;
	hr = NXMLSettings::GetIntFromMap(attributeMap.get(), SETTING_ID, id);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	int style;
	hr = NXMLSettings::GetIntFromMap(attributeMap.get(), SETTING_STYLE, style);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	int length;
	hr = NXMLSettings::GetIntFromMap(attributeMap.get(), SETTING_LENGTH, length);

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

std::vector<RebarBandStorageInfo> LoadFromNode(IXMLDOMNode *parentNode)
{
	wil::com_ptr_nothrow<IXMLDOMNode> childNode;
	auto queryString = wil::make_bstr_nothrow(L"./Toolbar");

	wil::com_ptr_nothrow<IXMLDOMNodeList> childNodes;
	HRESULT hr = parentNode->selectNodes(queryString.get(), &childNodes);

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

void SaveRebarBandInfo(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const RebarBandStorageInfo &bandInfo)
{
	wil::com_ptr_nothrow<IXMLDOMElement> bandNode;

	NXMLSettings::CreateElementNode(xmlDocument, &bandNode, parentNode, _T("Toolbar"), L"");

	NXMLSettings::AddAttributeToNode(xmlDocument, bandNode.get(), SETTING_ID,
		NXMLSettings::EncodeIntValue(bandInfo.id));
	NXMLSettings::AddAttributeToNode(xmlDocument, bandNode.get(), SETTING_STYLE,
		NXMLSettings::EncodeIntValue(bandInfo.style));
	NXMLSettings::AddAttributeToNode(xmlDocument, bandNode.get(), SETTING_LENGTH,
		NXMLSettings::EncodeIntValue(bandInfo.length));
}

void SaveToNode(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const std::vector<RebarBandStorageInfo> &rebarStorageInfo)
{
	for (const auto &bandStorageInfo : rebarStorageInfo)
	{
		SaveRebarBandInfo(xmlDocument, parentNode, bandStorageInfo);
	}
}

}

std::vector<RebarBandStorageInfo> Load(IXMLDOMDocument *xmlDocument)
{
	wil::com_ptr_nothrow<IXMLDOMNode> mainRebarNode;
	auto queryString = wil::make_bstr_nothrow(
		(std::wstring(L"/ExplorerPlusPlus/") + MAIN_REBAR_NODE_NAME).c_str());
	HRESULT hr = xmlDocument->selectSingleNode(queryString.get(), &mainRebarNode);

	if (FAILED(hr))
	{
		return {};
	}

	return LoadFromNode(mainRebarNode.get());
}

void Save(IXMLDOMDocument *xmlDocument, IXMLDOMElement *rootNode,
	const std::vector<RebarBandStorageInfo> &rebarStorageInfo)
{
	wil::com_ptr_nothrow<IXMLDOMElement> mainRebarNode;
	auto nodeName = wil::make_bstr_nothrow(MAIN_REBAR_NODE_NAME);
	HRESULT hr = xmlDocument->createElement(nodeName.get(), &mainRebarNode);

	if (FAILED(hr))
	{
		return;
	}

	SaveToNode(xmlDocument, mainRebarNode.get(), rebarStorageInfo);

	NXMLSettings::AppendChildToParent(mainRebarNode.get(), rootNode);
}

}
