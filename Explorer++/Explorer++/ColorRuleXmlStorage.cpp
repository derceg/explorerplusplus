// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ColorRule.h"
#include "ColorRuleModel.h"
#include "../Helper/XMLSettings.h"

namespace ColorRuleXmlStorage
{

namespace
{

const TCHAR COLOR_RULES_NODE_NAME[] = _T("ColorRules");

const TCHAR SETTING_DESCRIPTION[] = _T("name");
const TCHAR SETTING_FILENAME_PATTERN[] = _T("FilenamePattern");
const TCHAR SETTING_CASE_INSENSITIVE[] = _T("CaseInsensitive");
const TCHAR SETTING_ATTRIBUTES[] = _T("Attributes");

std::unique_ptr<ColorRule> LoadColorRule(IXMLDOMNode *parentNode)
{
	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> attributeMap;
	HRESULT hr = parentNode->get_attributes(&attributeMap);

	if (FAILED(hr))
	{
		return nullptr;
	}

	std::wstring description;
	hr = XMLSettings::GetStringFromMap(attributeMap.get(), SETTING_DESCRIPTION, description);

	if (FAILED(hr))
	{
		return nullptr;
	}

	std::wstring filenamePattern;
	hr = XMLSettings::GetStringFromMap(attributeMap.get(), SETTING_FILENAME_PATTERN,
		filenamePattern);

	if (FAILED(hr))
	{
		return nullptr;
	}

	bool caseInsensitive;
	hr = XMLSettings::GetBoolFromMap(attributeMap.get(), SETTING_CASE_INSENSITIVE, caseInsensitive);

	if (FAILED(hr))
	{
		return nullptr;
	}

	int attributes;
	hr = XMLSettings::GetIntFromMap(attributeMap.get(), SETTING_ATTRIBUTES, attributes);

	if (FAILED(hr))
	{
		return nullptr;
	}

	COLORREF color;
	hr = XMLSettings::ReadRgb(attributeMap.get(), color);

	if (FAILED(hr))
	{
		return nullptr;
	}

	return std::make_unique<ColorRule>(description, filenamePattern, caseInsensitive, attributes,
		color);
}

void LoadFromNode(IXMLDOMNode *parentNode, ColorRuleModel *model)
{
	wil::com_ptr_nothrow<IXMLDOMNode> childNode;
	auto queryString = wil::make_bstr_nothrow(L"./ColorRule");

	wil::com_ptr_nothrow<IXMLDOMNodeList> childNodes;
	HRESULT hr = parentNode->selectNodes(queryString.get(), &childNodes);

	if (FAILED(hr))
	{
		return;
	}

	while (childNodes->nextNode(&childNode) == S_OK)
	{
		auto colorRule = LoadColorRule(childNode.get());

		if (!colorRule)
		{
			continue;
		}

		model->AddItem(std::move(colorRule));
	}
}

void SaveColorRule(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const ColorRule *colorRule)
{
	wil::com_ptr_nothrow<IXMLDOMElement> colorRuleNode;
	XMLSettings::CreateElementNode(xmlDocument, &colorRuleNode, parentNode, _T("ColorRule"),
		colorRule->GetDescription().c_str());
	XMLSettings::AddAttributeToNode(xmlDocument, colorRuleNode.get(), SETTING_FILENAME_PATTERN,
		colorRule->GetFilterPattern().c_str());
	XMLSettings::AddAttributeToNode(xmlDocument, colorRuleNode.get(), SETTING_CASE_INSENSITIVE,
		XMLSettings::EncodeBoolValue(colorRule->GetFilterPatternCaseInsensitive()));
	XMLSettings::AddAttributeToNode(xmlDocument, colorRuleNode.get(), SETTING_ATTRIBUTES,
		XMLSettings::EncodeIntValue(colorRule->GetFilterAttributes()));
	XMLSettings::SaveRgb(xmlDocument, colorRuleNode.get(), colorRule->GetColor());
}

void SaveToNode(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const ColorRuleModel *model)
{
	for (const auto &colorRule : model->GetItems())
	{
		SaveColorRule(xmlDocument, parentNode, colorRule.get());
	}
}

}

void Load(IXMLDOMDocument *xmlDocument, ColorRuleModel *model)
{
	wil::com_ptr_nothrow<IXMLDOMNode> colorRulesNode;
	auto queryString = wil::make_bstr_nothrow(
		(std::wstring(L"/ExplorerPlusPlus/") + std::wstring(COLOR_RULES_NODE_NAME)).c_str());
	HRESULT hr = xmlDocument->selectSingleNode(queryString.get(), &colorRulesNode);

	if (hr != S_OK)
	{
		return;
	}

	model->RemoveAllItems();

	LoadFromNode(colorRulesNode.get(), model);
}

void Save(IXMLDOMDocument *xmlDocument, IXMLDOMNode *rootNode, const ColorRuleModel *model)
{
	wil::com_ptr_nothrow<IXMLDOMElement> colorRulesNode;
	auto nodeName = wil::make_bstr_nothrow(COLOR_RULES_NODE_NAME);
	HRESULT hr = xmlDocument->createElement(nodeName.get(), &colorRulesNode);

	if (FAILED(hr))
	{
		return;
	}

	SaveToNode(xmlDocument, colorRulesNode.get(), model);

	XMLSettings::AppendChildToParent(colorRulesNode.get(), rootNode);
}

}
