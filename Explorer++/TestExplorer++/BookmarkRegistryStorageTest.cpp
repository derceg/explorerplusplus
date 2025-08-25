// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/BookmarkRegistryStorage.h"
#include "BookmarkStorageTestHelper.h"
#include "Bookmarks/BookmarkTree.h"
#include "RegistryStorageTestHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class BookmarkRegistryStorageTest : public RegistryStorageTest
{
protected:
	void PerformLoadTest(const std::wstring &filename, BookmarkTree *referenceBookmarkTree,
		bool compareGuids)
	{
		ImportRegistryResource(filename);

		BookmarkTree loadedBookmarkTree;
		BookmarkRegistryStorage::Load(m_applicationTestKey.get(), &loadedBookmarkTree);

		CompareBookmarkTrees(&loadedBookmarkTree, referenceBookmarkTree, compareGuids);
	}
};

TEST_F(BookmarkRegistryStorageTest, V2Load)
{
	BookmarkTree referenceBookmarkTree;
	BuildV2LoadSaveReferenceTree(&referenceBookmarkTree);

	PerformLoadTest(L"bookmarks-v2.reg", &referenceBookmarkTree, true);
}

TEST_F(BookmarkRegistryStorageTest, V2LoadUpdateObserverInvokedOnce)
{
	ImportRegistryResource(L"bookmarks-v2.reg");

	BookmarkTree loadedBookmarkTree;
	BookmarkRegistryStorage::Load(m_applicationTestKey.get(), &loadedBookmarkTree);

	PerformV2UpdateObserverInvokedOnceTest(&loadedBookmarkTree);
}

TEST_F(BookmarkRegistryStorageTest, V2Save)
{
	BookmarkTree referenceBookmarkTree;
	BuildV2LoadSaveReferenceTree(&referenceBookmarkTree);

	BookmarkRegistryStorage::Save(m_applicationTestKey.get(), &referenceBookmarkTree);

	BookmarkTree loadedBookmarkTree;
	BookmarkRegistryStorage::Load(m_applicationTestKey.get(), &loadedBookmarkTree);

	CompareBookmarkTrees(&loadedBookmarkTree, &referenceBookmarkTree, true);
}

TEST_F(BookmarkRegistryStorageTest, V1BasicLoad)
{
	BookmarkTree referenceBookmarkTree;
	BuildV1BasicLoadReferenceTree(&referenceBookmarkTree);

	PerformLoadTest(L"bookmarks-v1.reg", &referenceBookmarkTree, false);
}

TEST_F(BookmarkRegistryStorageTest, V1NestedShowOnToolbarLoad)
{
	BookmarkTree referenceBookmarkTree;
	BuildV1NestedShowOnToolbarLoadReferenceTree(&referenceBookmarkTree);

	PerformLoadTest(L"bookmarks-v1-nested-show-on-toolbar.reg", &referenceBookmarkTree, false);
}

TEST_F(BookmarkRegistryStorageTest, V1LoadUpdateObserverInvokedOnce)
{
	ImportRegistryResource(L"bookmarks-v1.reg");

	BookmarkTree loadedBookmarkTree;
	BookmarkRegistryStorage::Load(m_applicationTestKey.get(), &loadedBookmarkTree);

	PerformV1UpdateObserverInvokedOnceTest(&loadedBookmarkTree);
}
