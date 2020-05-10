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

	void PerformLoadTest(const std::wstring &filename, BookmarkTree *referenceBookmarkTree, bool compareGuids)
	{
		std::wstring xmlFilePath = GetResourcePath(filename);
		auto xmlDocument = InitializeXmlDocument(xmlFilePath);
		ASSERT_TRUE(xmlDocument);

		BookmarkTree loadedBookmarkTree;
		BookmarkXmlStorage::Load(xmlDocument.get(), &loadedBookmarkTree);

		CompareBookmarkTrees(&loadedBookmarkTree, referenceBookmarkTree, compareGuids);
	}
};

TEST_F(BookmarkXmlStorageTest, V2Load)
{
	BookmarkTree referenceBookmarkTree;
	BuildV2LoadReferenceTree(&referenceBookmarkTree);

	PerformLoadTest(L"bookmarks-v2-config.xml", &referenceBookmarkTree, true);
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

	PerformLoadTest(L"bookmarks-v1-config-nested-show-on-toolbar.xml", &referenceBookmarkTree, false);
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