// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "XmlStorageTestHelper.h"
#include "../Helper/XMLSettings.h"

using namespace testing;

wil::com_ptr_nothrow<IXMLDOMDocument> XmlStorageTest::LoadXmlDocument(const std::wstring &filePath)
{
	wil::com_ptr_nothrow<IXMLDOMDocument> xmlDocument;
	LoadXmlDocumentHelper(filePath, xmlDocument);
	return xmlDocument;
}

void XmlStorageTest::LoadXmlDocumentHelper(const std::wstring &filePath,
	wil::com_ptr_nothrow<IXMLDOMDocument> &outputXmlDocument)
{
	outputXmlDocument = XMLSettings::CreateXmlDocument();
	ASSERT_THAT(outputXmlDocument, NotNull());

	VARIANT_BOOL status;
	auto filePathVariant = wil::make_variant_bstr_failfast(filePath.c_str());
	outputXmlDocument->load(filePathVariant, &status);
	ASSERT_EQ(status, VARIANT_TRUE);
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

	auto rootTag = wil::make_bstr_nothrow(L"ExplorerPlusPlus");
	wil::com_ptr_nothrow<IXMLDOMElement> root;
	xmlDocument->createElement(rootTag.get(), &root);
	XMLSettings::AppendChildToParent(root.get(), xmlDocument.get());

	outputXmlDocumentData = { std::move(xmlDocument), std::move(root) };
}
