// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "XmlStorageTestHelper.h"
#include "../Helper/XMLSettings.h"

wil::com_ptr_nothrow<IXMLDOMDocument> XmlStorageTest::LoadXmlDocument(const std::wstring &filePath)
{
	wil::com_ptr_nothrow<IXMLDOMDocument> xmlDocument;
	xmlDocument.attach(XMLSettings::DomFromCOM());

	if (!xmlDocument)
	{
		return nullptr;
	}

	VARIANT_BOOL status;
	VARIANT variantFilePath = XMLSettings::VariantString(filePath.c_str());
	xmlDocument->load(variantFilePath, &status);

	if (status != VARIANT_TRUE)
	{
		return nullptr;
	}

	return xmlDocument;
}

std::optional<XmlStorageTest::XmlDocumentData> XmlStorageTest::CreateXmlDocument()
{
	wil::com_ptr_nothrow<IXMLDOMDocument> xmlDocument;
	xmlDocument.attach(XMLSettings::DomFromCOM());

	if (!xmlDocument)
	{
		return {};
	}

	auto tag = wil::make_bstr_nothrow(L"xml");
	auto attribute = wil::make_bstr_nothrow(L"version='1.0'");
	wil::com_ptr_nothrow<IXMLDOMProcessingInstruction> processingInstruction;
	xmlDocument->createProcessingInstruction(tag.get(), attribute.get(), &processingInstruction);
	XMLSettings::AppendChildToParent(processingInstruction.get(), xmlDocument.get());

	auto rootTag = wil::make_bstr_nothrow(L"ExplorerPlusPlus");
	wil::com_ptr_nothrow<IXMLDOMElement> root;
	xmlDocument->createElement(rootTag.get(), &root);
	XMLSettings::AppendChildToParent(root.get(), xmlDocument.get());

	return XmlDocumentData{ std::move(xmlDocument), std::move(root) };
}
