// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "Bookmarks/BookmarkXmlStorage.h"
#include "BookmarkStorageHelper.h"
#include "Bookmarks/BookmarkTree.h"
#include "ResourceHelper.h"
#include "../Helper/XMLSettings.h"
#include <gtest/gtest.h>
#include <wil/com.h>
#include <wil/resource.h>
#include <optional>

using namespace testing;

struct XmlDocumentData
{
	wil::com_ptr_nothrow<IXMLDOMDocument> xmlDocument;
	wil::com_ptr_nothrow<IXMLDOMElement> root;
};

wil::com_ptr_nothrow<IXMLDOMDocument> LoadXmlDocument(const std::wstring &filePath);
std::optional<XmlDocumentData> CreateXmlDocument();

class BookmarkXmlStorageTest : public Test
{
protected:
	BookmarkXmlStorageTest()
	{
		CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	}

	~BookmarkXmlStorageTest()
	{
		CoUninitialize();
	}

	void PerformLoadTest(
		const std::wstring &filename, BookmarkTree *referenceBookmarkTree, bool compareGuids)
	{
		std::wstring xmlFilePath = GetResourcePath(filename);
		auto xmlDocument = LoadXmlDocument(xmlFilePath);
		ASSERT_TRUE(xmlDocument);

		BookmarkTree loadedBookmarkTree;
		BookmarkXmlStorage::Load(xmlDocument.get(), &loadedBookmarkTree);

		CompareBookmarkTrees(&loadedBookmarkTree, referenceBookmarkTree, compareGuids);
	}
};

TEST_F(BookmarkXmlStorageTest, V2Load)
{
	BookmarkTree referenceBookmarkTree;
	BuildV2LoadSaveReferenceTree(&referenceBookmarkTree);

	PerformLoadTest(L"bookmarks-v2-config.xml", &referenceBookmarkTree, true);
}

TEST_F(BookmarkXmlStorageTest, V2Save)
{
	BookmarkTree referenceBookmarkTree;
	BuildV2LoadSaveReferenceTree(&referenceBookmarkTree);

	auto xmlDocumentData = CreateXmlDocument();
	ASSERT_TRUE(xmlDocumentData);

	BookmarkXmlStorage::Save(
		xmlDocumentData->xmlDocument.get(), xmlDocumentData->root.get(), &referenceBookmarkTree, 1);

	BookmarkTree loadedBookmarkTree;
	BookmarkXmlStorage::Load(xmlDocumentData->xmlDocument.get(), &loadedBookmarkTree);

	CompareBookmarkTrees(&loadedBookmarkTree, &referenceBookmarkTree, true);
}

TEST_F(BookmarkXmlStorageTest, V1BasicLoad)
{
	BookmarkTree referenceBookmarkTree;
	BuildV1BasicLoadReferenceTree(&referenceBookmarkTree);

	PerformLoadTest(L"bookmarks-v1-config.xml", &referenceBookmarkTree, false);
}

TEST_F(BookmarkXmlStorageTest, V1NestedShowOnToolbarLoad)
{
	BookmarkTree referenceBookmarkTree;
	BuildV1NestedShowOnToolbarLoadReferenceTree(&referenceBookmarkTree);

	PerformLoadTest(
		L"bookmarks-v1-config-nested-show-on-toolbar.xml", &referenceBookmarkTree, false);
}

wil::com_ptr_nothrow<IXMLDOMDocument> LoadXmlDocument(const std::wstring &filePath)
{
	wil::com_ptr_nothrow<IXMLDOMDocument> xmlDocument;
	xmlDocument.attach(NXMLSettings::DomFromCOM());

	if (!xmlDocument)
	{
		return nullptr;
	}

	VARIANT_BOOL status;
	VARIANT variantFilePath = NXMLSettings::VariantString(filePath.c_str());
	xmlDocument->load(variantFilePath, &status);

	if (status != VARIANT_TRUE)
	{
		return nullptr;
	}

	return xmlDocument;
}

std::optional<XmlDocumentData> CreateXmlDocument()
{
	wil::com_ptr_nothrow<IXMLDOMDocument> xmlDocument;
	xmlDocument.attach(NXMLSettings::DomFromCOM());

	if (!xmlDocument)
	{
		return {};
	}

	auto tag = wil::make_bstr_nothrow(L"xml");
	auto attribute = wil::make_bstr_nothrow(L"version='1.0'");
	wil::com_ptr_nothrow<IXMLDOMProcessingInstruction> processingInstruction;
	xmlDocument->createProcessingInstruction(tag.get(), attribute.get(), &processingInstruction);
	NXMLSettings::AppendChildToParent(processingInstruction.get(), xmlDocument.get());

	auto rootTag = wil::make_bstr_nothrow(L"ExplorerPlusPlus");
	wil::com_ptr_nothrow<IXMLDOMElement> root;
	xmlDocument->createElement(rootTag.get(), &root);
	NXMLSettings::AppendChildToParent(root.get(), xmlDocument.get());

	return XmlDocumentData{ std::move(xmlDocument), std::move(root) };
}