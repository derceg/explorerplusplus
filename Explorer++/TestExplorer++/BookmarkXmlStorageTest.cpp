// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/BookmarkXmlStorage.h"
#include "BookmarkStorageTestHelper.h"
#include "Bookmarks/BookmarkTree.h"
#include "ResourceLoaderFake.h"
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
		auto xmlDocumentData = LoadXmlDocument(xmlFilePath);

		BookmarkTree loadedBookmarkTree(&m_resourceLoader);
		BookmarkXmlStorage::Load(xmlDocumentData.rootNode.get(), &loadedBookmarkTree);

		CompareBookmarkTrees(&loadedBookmarkTree, referenceBookmarkTree, compareGuids);
	}

	ResourceLoaderFake m_resourceLoader;
};

TEST_F(BookmarkXmlStorageTest, V2Load)
{
	BookmarkTree referenceBookmarkTree(&m_resourceLoader);
	BuildV2LoadSaveReferenceTree(&referenceBookmarkTree);

	PerformLoadTest(L"bookmarks-v2.xml", &referenceBookmarkTree, true);
}

TEST_F(BookmarkXmlStorageTest, V2LoadUpdateObserverInvokedOnce)
{
	std::wstring xmlFilePath = GetResourcePath(L"bookmarks-v2.xml");
	auto xmlDocumentData = LoadXmlDocument(xmlFilePath);

	BookmarkTree loadedBookmarkTree(&m_resourceLoader);
	BookmarkXmlStorage::Load(xmlDocumentData.rootNode.get(), &loadedBookmarkTree);

	PerformV2UpdateObserverInvokedOnceTest(&loadedBookmarkTree);
}

TEST_F(BookmarkXmlStorageTest, V2Save)
{
	BookmarkTree referenceBookmarkTree(&m_resourceLoader);
	BuildV2LoadSaveReferenceTree(&referenceBookmarkTree);

	auto xmlDocumentData = CreateXmlDocument();

	BookmarkXmlStorage::Save(xmlDocumentData.xmlDocument.get(), xmlDocumentData.rootNode.get(),
		&referenceBookmarkTree);

	BookmarkTree loadedBookmarkTree(&m_resourceLoader);
	BookmarkXmlStorage::Load(xmlDocumentData.rootNode.get(), &loadedBookmarkTree);

	CompareBookmarkTrees(&loadedBookmarkTree, &referenceBookmarkTree, true);
}

TEST_F(BookmarkXmlStorageTest, V1BasicLoad)
{
	BookmarkTree referenceBookmarkTree(&m_resourceLoader);
	BuildV1BasicLoadReferenceTree(&referenceBookmarkTree);

	PerformLoadTest(L"bookmarks-v1.xml", &referenceBookmarkTree, false);
}

TEST_F(BookmarkXmlStorageTest, V1NestedShowOnToolbarLoad)
{
	BookmarkTree referenceBookmarkTree(&m_resourceLoader);
	BuildV1NestedShowOnToolbarLoadReferenceTree(&referenceBookmarkTree);

	PerformLoadTest(L"bookmarks-v1-nested-show-on-toolbar.xml", &referenceBookmarkTree, false);
}

TEST_F(BookmarkXmlStorageTest, V1LoadUpdateObserverInvokedOnce)
{
	std::wstring xmlFilePath = GetResourcePath(L"bookmarks-v1.xml");
	auto xmlDocumentData = LoadXmlDocument(xmlFilePath);

	BookmarkTree loadedBookmarkTree(&m_resourceLoader);
	BookmarkXmlStorage::Load(xmlDocumentData.rootNode.get(), &loadedBookmarkTree);

	PerformV1UpdateObserverInvokedOnceTest(&loadedBookmarkTree);
}
