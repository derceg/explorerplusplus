// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/BookmarkXmlStorage.h"
#include "BookmarkStorageTestHelper.h"
#include "Bookmarks/BookmarkTree.h"
#include "ResourceTestHelper.h"
#include "XmlStorageTestHelper.h"
#include "../Helper/XMLSettings.h"
#include <gtest/gtest.h>

using namespace testing;

class BookmarkXmlStorageTest : public XmlStorageTest
{
protected:
	void PerformLoadTest(const std::wstring &filename, BookmarkTree *referenceBookmarkTree,
		bool compareGuids)
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

	BookmarkXmlStorage::Save(xmlDocumentData->xmlDocument.get(), xmlDocumentData->root.get(),
		&referenceBookmarkTree, 1);

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

	PerformLoadTest(L"bookmarks-v1-config-nested-show-on-toolbar.xml", &referenceBookmarkTree,
		false);
}
