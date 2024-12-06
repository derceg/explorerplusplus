// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ApplicationToolbarXmlStorage.h"
#include "Application.h"
#include "ApplicationModel.h"
#include "../Helper/XMLSettings.h"
#include <wil/com.h>

namespace Applications
{

namespace ApplicationToolbarXmlStorage
{

namespace
{

const wchar_t APPLICATION_TOOLBAR_NODE_NAME[] = L"ApplicationToolbar";

const wchar_t SETTING_NAME[] = L"name";
const wchar_t SETTING_COMMAND[] = L"Command";
const wchar_t SETTING_SHOW_NAME_ON_TOOLBAR[] = L"ShowNameOnToolbar";

std::unique_ptr<Application> LoadApplication(IXMLDOMNode *parentNode)
{
	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> attributeMap;
	HRESULT hr = parentNode->get_attributes(&attributeMap);

	if (FAILED(hr))
	{
		return nullptr;
	}

	std::wstring name;
	hr = XMLSettings::GetStringFromMap(attributeMap.get(), SETTING_NAME, name);

	if (FAILED(hr))
	{
		return nullptr;
	}

	std::wstring command;
	hr = XMLSettings::GetStringFromMap(attributeMap.get(), SETTING_COMMAND, command);

	if (FAILED(hr))
	{
		return nullptr;
	}

	bool showNameOnToolbar;
	hr = XMLSettings::GetBoolFromMap(attributeMap.get(), SETTING_SHOW_NAME_ON_TOOLBAR,
		showNameOnToolbar);

	if (FAILED(hr))
	{
		return nullptr;
	}

	return std::make_unique<Application>(name, command, showNameOnToolbar);
}

void LoadFromNode(IXMLDOMNode *parentNode, ApplicationModel *model)
{
	wil::com_ptr_nothrow<IXMLDOMNode> childNode;
	auto queryString = wil::make_bstr_nothrow(L"./ApplicationButton");

	wil::com_ptr_nothrow<IXMLDOMNodeList> childNodes;
	HRESULT hr = parentNode->selectNodes(queryString.get(), &childNodes);

	if (FAILED(hr))
	{
		return;
	}

	while (childNodes->nextNode(&childNode) == S_OK)
	{
		auto application = LoadApplication(childNode.get());

		if (!application)
		{
			continue;
		}

		model->AddItem(std::move(application));
	}
}

void SaveApplication(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const Application *application)
{
	wil::com_ptr_nothrow<IXMLDOMElement> applicationNode;
	XMLSettings::CreateElementNode(xmlDocument, &applicationNode, parentNode,
		_T("ApplicationButton"), application->GetName().c_str());
	XMLSettings::AddAttributeToNode(xmlDocument, applicationNode.get(), SETTING_COMMAND,
		application->GetCommand().c_str());
	XMLSettings::AddAttributeToNode(xmlDocument, applicationNode.get(),
		SETTING_SHOW_NAME_ON_TOOLBAR,
		XMLSettings::EncodeBoolValue(application->GetShowNameOnToolbar()));
}

void SaveToNode(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const ApplicationModel *model)
{
	for (const auto &application : model->GetItems())
	{
		SaveApplication(xmlDocument, parentNode, application.get());
	}
}

}

void Load(IXMLDOMNode *rootNode, ApplicationModel *model)
{
	wil::com_ptr_nothrow<IXMLDOMNode> applicationToolbarNode;
	auto queryString = wil::make_bstr_nothrow(APPLICATION_TOOLBAR_NODE_NAME);
	HRESULT hr = rootNode->selectSingleNode(queryString.get(), &applicationToolbarNode);

	if (hr != S_OK)
	{
		return;
	}

	LoadFromNode(applicationToolbarNode.get(), model);
}

void Save(IXMLDOMDocument *xmlDocument, IXMLDOMNode *rootNode, const ApplicationModel *model)
{
	wil::com_ptr_nothrow<IXMLDOMElement> applicationToolbarNode;
	auto nodeName = wil::make_bstr_nothrow(APPLICATION_TOOLBAR_NODE_NAME);
	HRESULT hr = xmlDocument->createElement(nodeName.get(), &applicationToolbarNode);

	if (FAILED(hr))
	{
		return;
	}

	SaveToNode(xmlDocument, applicationToolbarNode.get(), model);

	XMLSettings::AppendChildToParent(applicationToolbarNode.get(), rootNode);
}

}

}
