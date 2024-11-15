// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "XmlStorageTestHelper.h"
#include "Storage.h"
#include "../Helper/XMLSettings.h"

using namespace testing;

XmlStorageTest::XmlDocumentData XmlStorageTest::LoadXmlDocument(const std::wstring &filePath)
{
	XmlDocumentData xmlDocumentData;
	LoadXmlDocumentHelper(filePath, xmlDocumentData);
	return xmlDocumentData;
}

void XmlStorageTest::LoadXmlDocumentHelper(const std::wstring &filePath,
	XmlDocumentData &outputXmlDocumentData)
{
	auto xmlDocument = XMLSettings::CreateXmlDocument();
	ASSERT_THAT(xmlDocument, NotNull());

	VARIANT_BOOL status;
	auto filePathVariant = wil::make_variant_bstr_failfast(filePath.c_str());
	xmlDocument->load(filePathVariant, &status);
	ASSERT_EQ(status, VARIANT_TRUE);

	wil::com_ptr_nothrow<IXMLDOMNode> rootNode;
	auto query = wil::make_bstr_nothrow(Storage::CONFIG_FILE_ROOT_NODE_NAME);
	HRESULT hr = xmlDocument->selectSingleNode(query.get(), &rootNode);
	ASSERT_EQ(hr, S_OK);

	outputXmlDocumentData = { xmlDocument, rootNode };
}

XmlStorageTest::XmlDocumentData XmlStorageTest::CreateXmlDocument()
{
	XmlDocumentData xmlDocumentData;
	CreateXmlDocumentHelper(xmlDocumentData);
	return xmlDocumentData;
}

void XmlStorageTest::CreateXmlDocumentHelper(XmlDocumentData &outputXmlDocumentData)
{
	auto xmlDocument = XMLSettings::CreateXmlDocument();
	ASSERT_THAT(xmlDocument, NotNull());

	auto tag = wil::make_bstr_nothrow(L"xml");
	auto attribute = wil::make_bstr_nothrow(L"version='1.0'");
	wil::com_ptr_nothrow<IXMLDOMProcessingInstruction> processingInstruction;
	xmlDocument->createProcessingInstruction(tag.get(), attribute.get(), &processingInstruction);
	XMLSettings::AppendChildToParent(processingInstruction.get(), xmlDocument.get());

	auto rootTag = wil::make_bstr_nothrow(Storage::CONFIG_FILE_ROOT_NODE_NAME);
	wil::com_ptr_nothrow<IXMLDOMElement> rootNode;
	xmlDocument->createElement(rootTag.get(), &rootNode);
	XMLSettings::AppendChildToParent(rootNode.get(), xmlDocument.get());

	outputXmlDocumentData = { xmlDocument, rootNode };
}
