// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ConfigRegistryStorage.h"
#include "Config.h"
#include "ConfigStorageTestHelper.h"
#include "RegistryStorageTestHelper.h"
#include <gtest/gtest.h>

class ConfigRegistryStorageTest : public RegistryStorageTest
{
};

TEST_F(ConfigRegistryStorageTest, SaveLoad)
{
	auto referenceConfig = ConfigStorageTestHelper::BuildReference();
	ConfigRegistryStorage::Save(m_applicationTestKey.get(), referenceConfig);

	Config loadedConfig;
	ConfigRegistryStorage::Load(m_applicationTestKey.get(), loadedConfig);

	EXPECT_EQ(loadedConfig, referenceConfig);
}

TEST_F(ConfigRegistryStorageTest, OpenZipFilesSettingMigration)
{
	ImportRegistryResource(L"config-migration.reg");

	Config loadedConfig;
	ConfigRegistryStorage::Load(m_applicationTestKey.get(), loadedConfig);

	EXPECT_TRUE(loadedConfig.openContainerFiles);
}
