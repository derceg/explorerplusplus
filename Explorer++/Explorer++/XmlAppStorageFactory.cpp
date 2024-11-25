// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "XmlAppStorageFactory.h"
#include "XmlAppStorage.h"
#include "../Helper/XMLSettings.h"

std::unique_ptr<XmlAppStorage> XmlAppStorageFactory::MaybeCreate(const std::wstring &configFilePath,
	Storage::OperationType operationType)
{
	if (operationType == Storage::OperationType::Load)
	{
		return BuildForLoad(configFilePath);
	}
	else
	{
		return BuildForSave(configFilePath);
	}
}

std::unique_ptr<XmlAppStorage> XmlAppStorageFactory::BuildForLoad(
	const std::wstring &configFilePath)
{
	auto xmlDocument = XMLSettings::CreateXmlDocument();

	if (!xmlDocument)
	{
		return nullptr;
	}

	auto configFilePathVariant = wil::make_variant_bstr_failfast(configFilePath.c_str());
	VARIANT_BOOL status;
	xmlDocument->load(configFilePathVariant, &status);

	if (status != VARIANT_TRUE)
	{
		return nullptr;
	}

	wil::com_ptr_nothrow<IXMLDOMNode> rootNode;
	auto query = wil::make_bstr_failfast(Storage::CONFIG_FILE_ROOT_NODE_NAME);
	HRESULT hr = xmlDocument->selectSingleNode(query.get(), &rootNode);

	if (hr != S_OK)
	{
		return nullptr;
	}

	return std::make_unique<XmlAppStorage>(xmlDocument, rootNode, configFilePath,
		Storage::OperationType::Load);
}

std::unique_ptr<XmlAppStorage> XmlAppStorageFactory::BuildForSave(
	const std::wstring &configFilePath)
{
	auto xmlDocument = XMLSettings::CreateXmlDocument();

	if (!xmlDocument)
	{
		return nullptr;
	}

	auto tag = wil::make_bstr_failfast(L"xml");
	auto attribute = wil::make_bstr_failfast(L"version='1.0'");
	wil::com_ptr_nothrow<IXMLDOMProcessingInstruction> processingInstruction;
	HRESULT hr = xmlDocument->createProcessingInstruction(tag.get(), attribute.get(),
		&processingInstruction);

	if (hr != S_OK)
	{
		return nullptr;
	}

	XMLSettings::AppendChildToParent(processingInstruction.get(), xmlDocument.get());

	wil::com_ptr_nothrow<IXMLDOMComment> comment;
	auto commentText = wil::make_bstr_failfast(L" Preference file for Explorer++ ");
	hr = xmlDocument->createComment(commentText.get(), &comment);

	if (hr != S_OK)
	{
		return nullptr;
	}

	XMLSettings::AppendChildToParent(comment.get(), xmlDocument.get());

	auto rootTag = wil::make_bstr_nothrow(Storage::CONFIG_FILE_ROOT_NODE_NAME);
	wil::com_ptr_nothrow<IXMLDOMElement> rootNode;
	hr = xmlDocument->createElement(rootTag.get(), &rootNode);

	if (hr != S_OK)
	{
		return nullptr;
	}

	XMLSettings::AppendChildToParent(rootNode.get(), xmlDocument.get());

	return std::make_unique<XmlAppStorage>(xmlDocument, rootNode, configFilePath,
		Storage::OperationType::Save);
}
