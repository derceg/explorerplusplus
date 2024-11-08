// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "DefaultColumnRegistryStorage.h"
#include "ColumnStorageTestHelper.h"
#include "RegistryStorageTestHelper.h"
#include "ShellBrowser/FolderSettings.h"
#include <gtest/gtest.h>

class DefaultColumnRegistryStorageTest : public RegistryStorageTest
{
};

TEST_F(DefaultColumnRegistryStorageTest, Load)
{
	auto referenceColumns = BuildFolderColumnsLoadSaveReference();

	ImportRegistryResource(L"default-columns.reg");

	FolderColumns loadedColumns;
	DefaultColumnRegistryStorage::Load(m_applicationTestKey.get(), loadedColumns);

	EXPECT_EQ(loadedColumns, referenceColumns);
}

TEST_F(DefaultColumnRegistryStorageTest, Save)
{
	auto referenceColumns = BuildFolderColumnsLoadSaveReference();

	DefaultColumnRegistryStorage::Save(m_applicationTestKey.get(), referenceColumns);

	FolderColumns loadedColumns;
	DefaultColumnRegistryStorage::Load(m_applicationTestKey.get(), loadedColumns);

	EXPECT_EQ(loadedColumns, referenceColumns);
}
