// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ColumnRegistryStorage.h"
#include "ColumnStorageTestHelper.h"
#include "RegistryStorageTestHelper.h"
#include "ShellBrowser/FolderSettings.h"
#include <gtest/gtest.h>
#include <wil/registry.h>

using namespace testing;

class ColumnRegistryStorageTest : public RegistryStorageTest
{
protected:
	static inline const WCHAR COLUMNS_KEY_NAME[] = L"Columns";
};

TEST_F(ColumnRegistryStorageTest, Load)
{
	auto referenceColumns = BuildFolderColumnsLoadSaveReference();

	ImportRegistryResource(L"columns.reg");

	wil::unique_hkey columnsKey;
	HRESULT hr = wil::reg::open_unique_key_nothrow(m_applicationTestKey.get(), COLUMNS_KEY_NAME,
		columnsKey, wil::reg::key_access::read);
	ASSERT_HRESULT_SUCCEEDED(hr);

	FolderColumns loadedColumns;
	ColumnRegistryStorage::LoadAllColumnSets(columnsKey.get(), loadedColumns);

	EXPECT_EQ(loadedColumns, referenceColumns);
}

TEST_F(ColumnRegistryStorageTest, Save)
{
	auto referenceColumns = BuildFolderColumnsLoadSaveReference();

	wil::unique_hkey columnsKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(m_applicationTestKey.get(), COLUMNS_KEY_NAME,
		columnsKey, wil::reg::key_access::readwrite);
	ASSERT_HRESULT_SUCCEEDED(hr);

	ColumnRegistryStorage::SaveAllColumnSets(columnsKey.get(), referenceColumns);

	FolderColumns loadedColumns;
	ColumnRegistryStorage::LoadAllColumnSets(columnsKey.get(), loadedColumns);

	EXPECT_EQ(loadedColumns, referenceColumns);
}
