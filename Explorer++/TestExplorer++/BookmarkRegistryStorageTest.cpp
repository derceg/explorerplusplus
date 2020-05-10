// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "Bookmarks/BookmarkRegistryStorage.h"
#include "BookmarkStorageHelper.h"
#include "Bookmarks/BookmarkTree.h"
#include "ResourceHelper.h"
#include <gtest/gtest.h>
#include <Shlwapi.h>
#include <shellapi.h>

using namespace testing;

const TCHAR g_applicationTestKey[] = _T("Software\\Explorer++Test");

class BookmarkRegistryStorageTest : public Test
{
protected:
	void TearDown() override
	{
		LSTATUS result = SHDeleteKey(HKEY_CURRENT_USER, g_applicationTestKey);
		ASSERT_EQ(result, ERROR_SUCCESS);
	}

	void PerformLoadTest(
		const std::wstring &filename, BookmarkTree *referenceBookmarkTree, bool compareGuids)
	{
		ImportRegistryResource(filename);

		BookmarkTree loadedBookmarkTree;
		BookmarkRegistryStorage::Load(g_applicationTestKey, &loadedBookmarkTree);

		CompareBookmarkTrees(&loadedBookmarkTree, referenceBookmarkTree, compareGuids);
	}

	void ImportRegistryResource(const std::wstring &filename)
	{
		std::wstring command = L"/c reg import " + filename;
		auto resourcesPath = GetResourcesDirectoryPath();

		SHELLEXECUTEINFO shellExecuteInfo;
		shellExecuteInfo.cbSize = sizeof(shellExecuteInfo);
		shellExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
		shellExecuteInfo.hwnd = nullptr;
		shellExecuteInfo.lpVerb = L"open";
		shellExecuteInfo.lpFile = L"cmd.exe";
		shellExecuteInfo.lpParameters = command.c_str();
		shellExecuteInfo.lpDirectory = resourcesPath.c_str();
		shellExecuteInfo.nShow = SW_HIDE;
		shellExecuteInfo.hInstApp = nullptr;
		BOOL result = ShellExecuteEx(&shellExecuteInfo);
		ASSERT_TRUE(result);

		WaitForSingleObject(shellExecuteInfo.hProcess, INFINITE);
		CloseHandle(shellExecuteInfo.hProcess);
	}
};

TEST_F(BookmarkRegistryStorageTest, V2Load)
{
	BookmarkTree referenceBookmarkTree;
	BuildV2LoadSaveReferenceTree(&referenceBookmarkTree);

	PerformLoadTest(L"bookmarks-v2.reg", &referenceBookmarkTree, true);
}

TEST_F(BookmarkRegistryStorageTest, V2Save)
{
	BookmarkTree referenceBookmarkTree;
	BuildV2LoadSaveReferenceTree(&referenceBookmarkTree);

	BookmarkRegistryStorage::Save(g_applicationTestKey, &referenceBookmarkTree);

	BookmarkTree loadedBookmarkTree;
	BookmarkRegistryStorage::Load(g_applicationTestKey, &loadedBookmarkTree);

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