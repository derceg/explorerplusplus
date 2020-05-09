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

using namespace testing;

wil::com_ptr<IXMLDOMDocument> InitializeXmlDocument(const std::wstring &filePath);

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
};

TEST_F(BookmarkXmlStorageTest, V1BasicLoad)
{
	BookmarkTree referenceBookmarkTree;
	BuildV1BasicLoadReferenceTree(&referenceBookmarkTree);

	std::wstring xmlFilePath = GetResourcePath(L"bookmarks-v1-config.xml");
	auto xmlDocument = InitializeXmlDocument(xmlFilePath);
	ASSERT_TRUE(xmlDocument);

	BookmarkTree loadedBookmarkTree;
	BookmarkXmlStorage::Load(xmlDocument.get(), &loadedBookmarkTree);

	CompareBookmarkTrees(&loadedBookmarkTree, &referenceBookmarkTree);
}

TEST_F(BookmarkXmlStorageTest, V1NestedShowOnToolbarLoad)
{
	BookmarkTree referenceBookmarkTree;
	BuildV1NestedShowOnToolbarLoadReferenceTree(&referenceBookmarkTree);

	std::wstring xmlFilePath = GetResourcePath(L"bookmarks-v1-config-nested-show-on-toolbar.xml");
	auto xmlDocument = InitializeXmlDocument(xmlFilePath);
	ASSERT_TRUE(xmlDocument);

	BookmarkTree loadedBookmarkTree;
	BookmarkXmlStorage::Load(xmlDocument.get(), &loadedBookmarkTree);

	CompareBookmarkTrees(&loadedBookmarkTree, &referenceBookmarkTree);
}

wil::com_ptr<IXMLDOMDocument> InitializeXmlDocument(const std::wstring &filePath)
{
	wil::com_ptr<IXMLDOMDocument> xmlDocument(NXMLSettings::DomFromCOM());

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