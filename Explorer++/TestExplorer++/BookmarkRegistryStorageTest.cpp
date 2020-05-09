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

	void TearDown() override
	{
		LSTATUS result = SHDeleteKey(HKEY_CURRENT_USER, g_applicationTestKey);
		ASSERT_EQ(result, ERROR_SUCCESS);
	}
};

TEST_F(BookmarkRegistryStorageTest, V1BasicLoad)
{
	ImportRegistryResource(L"bookmarks-v1.reg");

	BookmarkTree referenceBookmarkTree;
	BuildV1BasicLoadReferenceTree(&referenceBookmarkTree);

	BookmarkTree loadedBookmarkTree;
	BookmarkRegistryStorage::Load(g_applicationTestKey, &loadedBookmarkTree);

	CompareBookmarkTrees(&loadedBookmarkTree, &referenceBookmarkTree);
}

TEST_F(BookmarkRegistryStorageTest, V1NestedShowOnToolbarLoad)
{
	ImportRegistryResource(L"bookmarks-v1-nested-show-on-toolbar.reg");

	BookmarkTree referenceBookmarkTree;
	BuildV1NestedShowOnToolbarLoadReferenceTree(&referenceBookmarkTree);

	BookmarkTree loadedBookmarkTree;
	BookmarkRegistryStorage::Load(g_applicationTestKey, &loadedBookmarkTree);

	CompareBookmarkTrees(&loadedBookmarkTree, &referenceBookmarkTree);
}